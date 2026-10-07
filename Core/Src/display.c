#include "display.h"
#include "font.h"
#include "util.h"

/* ----------------- private helpers ----------------- */

static inline void SPI_WaitAndSend(uint8_t byte) {
    while (!LL_SPI_IsActiveFlag_TXE(DISP_SPI)) {} /* wait til transmit buffer empty */
    LL_SPI_TransmitData8(DISP_SPI, byte);         /* push byte to data register (DR) */
}

static inline void SPI_Send16(uint16_t val) {
    /* STM32 is little-endian, ST7789V is big-endian -> send MSB first */
    SPI_WaitAndSend(val >> 8);   /* MSB */
    SPI_WaitAndSend(val & 0xFF); /* LSB */
}

static inline void SPI_BeginCommand(void) {
    LL_GPIO_ResetOutputPin(DISP_PORT_B, DISP_DC_PIN); /* DC low, command mode */
    LL_GPIO_ResetOutputPin(DISP_PORT_A, DISP_CS_PIN); /* CS low, activate screen */
}

static inline void SPI_BeginData(void) {
    LL_GPIO_SetOutputPin(DISP_PORT_B, DISP_DC_PIN);   /* DC high, data mode */
    LL_GPIO_ResetOutputPin(DISP_PORT_A, DISP_CS_PIN); /* CS low, activate screen */
}

static inline void SPI_End(void) {
    while (LL_SPI_IsActiveFlag_BSY(DISP_SPI)) {}    /* wait til bytes finished sending */
    LL_GPIO_SetOutputPin(DISP_PORT_A, DISP_CS_PIN); /* CS high, unlock bus */
}

/* ----------------- public implementation ----------------- */

void Display_InitHardware(void) {
    /* wake up GPIOA, GPIOB and SPI clocks */
    DISP_PORT_A_CLK_EN();
    DISP_PORT_B_CLK_EN();
    DISP_SPI_CLK_EN();

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* configure SPI (SCK, MOSI) on port A */
    GPIO_InitStruct.Pin        = DISP_SCK_PIN | DISP_MOSI_PIN;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate  = LL_GPIO_AF_5;
    LL_GPIO_Init(DISP_PORT_A, &GPIO_InitStruct);

    /* configure CS on port A */
    GPIO_InitStruct.Pin       = DISP_CS_PIN;
    GPIO_InitStruct.Mode      = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Alternate = 0; /* GPIO doesn't use Alt Function */
    LL_GPIO_Init(DISP_PORT_A, &GPIO_InitStruct);

    /* configure DC, RST on port B */
    GPIO_InitStruct.Pin = DISP_DC_PIN | DISP_RST_PIN;
    LL_GPIO_Init(DISP_PORT_B, &GPIO_InitStruct);

    /* CS high (inactive), RST high (no reset) by default */
    LL_GPIO_SetOutputPin(DISP_PORT_A, DISP_CS_PIN);
    LL_GPIO_SetOutputPin(DISP_PORT_B, DISP_RST_PIN);

    LL_SPI_InitTypeDef SPI_InitStruct = {0};

    SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;            /* bidir transmission */
    SPI_InitStruct.Mode              = LL_SPI_MODE_MASTER;            /* STM32 controlled */
    SPI_InitStruct.DataWidth         = LL_SPI_DATAWIDTH_8BIT;         /* ST7789V expects 8b calls */
    SPI_InitStruct.ClockPolarity     = LL_SPI_POLARITY_LOW;           /* clock low on standby */
    SPI_InitStruct.ClockPhase        = LL_SPI_PHASE_1EDGE;            /* sampling on first edge */
    SPI_InitStruct.NSS               = LL_SPI_NSS_SOFT;               /* controlled with CS */
    SPI_InitStruct.BaudRate          = LL_SPI_BAUDRATEPRESCALER_DIV2; /* max throughput */
    SPI_InitStruct.BitOrder          = LL_SPI_MSB_FIRST;              /* ST7789V expects MSB */

    LL_SPI_Init(DISP_SPI, &SPI_InitStruct);

    LL_SPI_Enable(DISP_SPI);
}

void Display_SendCommand(uint8_t cmd) {
    SPI_BeginCommand();
    SPI_WaitAndSend(cmd);
    SPI_End();
}

void Display_SendData(uint8_t data) {
    SPI_BeginData();
    SPI_WaitAndSend(data);
    SPI_End();
}

void Display_Init(void) {
    /* hw reset */
    LL_GPIO_ResetOutputPin(DISP_PORT_B, DISP_RST_PIN);
    LL_mDelay(ST7789V_DELAY_HWRESET);
    LL_GPIO_SetOutputPin(DISP_PORT_B, DISP_RST_PIN);
    LL_mDelay(ST7789V_DELAY_HWRESET);

    /* sw reset */
    Display_SendCommand(ST7789V_CMD_SWRESET);
    LL_mDelay(ST7789V_DELAY_SWRESET);

    /* wake up */
    Display_SendCommand(ST7789V_CMD_SLPOUT);
    LL_mDelay(ST7789V_DELAY_SLPOUT);

    /* set pixel format (RGB565) */
    Display_SendCommand(ST7789V_CMD_COLMOD);
    Display_SendData(ST7789V_COLMOD_16BIT);

    /* set portrait mode */
    Display_SendCommand(ST7789V_CMD_MADCTL);
    Display_SendData(ST7789V_MADCTL_RGB);

    /* inverse colors since ST7789V is IPS */
    Display_SendCommand(ST7789V_CMD_INVON);

    /* enable normal display mode */
    Display_SendCommand(ST7789V_CMD_NORON);
    LL_mDelay(ST7789V_DELAY_STATE);

    /* enable the display itself */
    Display_SendCommand(ST7789V_CMD_DISPON);
    LL_mDelay(ST7789V_DELAY_STATE);
}

void Display_SetWindow(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y) {
    STMP32_ASSERT(start_x < ST7789V_DISP_WID && end_x < ST7789V_DISP_WID);
    STMP32_ASSERT(start_y < ST7789V_DISP_HEI && end_y < ST7789V_DISP_HEI);
    STMP32_ASSERT(start_x <= end_x && start_y <= end_y);

    /* send x-values byte-by-byte */
    Display_SendCommand(ST7789V_CMD_CASET);
    Display_SendData(start_x >> 8);
    Display_SendData(start_x & 0xFF);
    Display_SendData(end_x >> 8);
    Display_SendData(end_x & 0xFF);

    /* send y-values byte-by-byte */
    Display_SendCommand(ST7789V_CMD_RASET);
    Display_SendData(start_y >> 8);
    Display_SendData(start_y & 0xFF);
    Display_SendData(end_y >> 8);
    Display_SendData(end_y & 0xFF);

    /* set screen mode to write data to pixel buffer */
    Display_SendCommand(ST7789V_CMD_RAMWR);
}

void Display_FillRect(uint16_t x, uint16_t y, uint16_t wid, uint16_t hei, uint16_t color) {
    /* clamp wid/hei values */
    if (x + wid > ST7789V_DISP_WID)
        wid = ST7789V_DISP_WID - x;
    if (y + hei > ST7789V_DISP_HEI)
        hei = ST7789V_DISP_HEI - y;

    uint32_t px_cnt = wid * hei;

    /* instead of Display_SendData on every px, handle it directly manually here */
    Display_SetWindow(x, y, x + wid - 1, y + hei - 1); /* open window of rect size */
    SPI_BeginData();

    for (uint32_t i = 0; i < px_cnt; ++i)
        SPI_Send16(color);

    SPI_End();
}

void Display_FillScreen(uint16_t color) {
    Display_FillRect(0, 0, ST7789V_DISP_WID, ST7789V_DISP_HEI, color);
}

void Display_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    Display_FillRect(x, y, 1, 1, color);
}

void Display_DrawChar(uint16_t x, uint16_t y, char c, uint16_t fg_color, uint16_t bg_color,
                      uint8_t scale) {
    STMP32_ASSERT(scale != 0);
    if (c < 32 || c > 126)
        return; /* draw only ASCII chars */

    uint8_t  font_idx = c - ' ';                          /* get bitmap table index */
    uint16_t char_wid = 8 * scale, char_hei = 16 * scale; /* def size is 8x16px, mult by scale */

    if (x + char_wid > ST7789V_DISP_WID || y + char_hei > ST7789V_DISP_HEI)
        return;

    /* instead of Display_SendData on every px, handle it directly manually here */
    Display_SetWindow(x, y, x + char_wid - 1, y + char_hei - 1); /* bind to char-sized window */
    SPI_BeginData();

    for (uint16_t row = 0; row < 16; ++row) {
        uint8_t row_data = terminus_8x16[font_idx][row];

        for (uint8_t scale_y = 0; scale_y < scale; ++scale_y)
            for (uint8_t col = 0; col < 8; ++col) {
                uint8_t is_px = row_data & (0x80 >> col);

                for (uint8_t scale_x = 0; scale_x < scale; ++scale_x)
                    if (is_px)
                        SPI_Send16(fg_color);
                    else
                        SPI_Send16(bg_color);
            }
    }

    SPI_End();
}

void Display_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t fg_color,
                        uint16_t bg_color, uint8_t scale) {
    STMP32_ASSERT(scale != 0);

    uint16_t len = 0;
    while (str[len] != '\0')
        len++;
    if (len == 0)
        return;

    uint16_t total_wid = len * 8 * scale;
    uint16_t total_hei = 16 * scale;

    if (x + total_wid > ST7789V_DISP_WID)
        total_wid = ST7789V_DISP_WID - x;
    if (y + total_hei > ST7789V_DISP_HEI)
        total_hei = ST7789V_DISP_HEI - y;

    /* instead of Display_SendData on every px, handle it directly manually here */
    Display_SetWindow(x, y, x + total_wid - 1, y + total_hei - 1);
    SPI_BeginData();

    for (uint8_t row = 0; row < 16; ++row)
        for (uint8_t scale_y = 0; scale_y < scale; ++scale_y) {
            if ((row * scale + scale_y) >= total_hei)
                break;

            uint16_t cur_x = 0;

            for (uint16_t i = 0; i < len; ++i) {
                uint8_t font_idx = str[i] - ' ';
                uint8_t row_data = terminus_8x16[font_idx][row];

                for (uint8_t col = 0; col < 8; ++col) {
                    uint8_t is_px = row_data & (0x80 >> col);

                    for (uint8_t scale_x = 0; scale_x < scale; ++scale_x) {
                        if (cur_x >= total_wid)
                            break;

                        if (is_px)
                            SPI_Send16(fg_color);
                        else
                            SPI_Send16(bg_color);

                        cur_x++;
                    }
                }
            }
        }

    SPI_End();
}
