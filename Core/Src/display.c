#include "display.h"
#include "font.h"
#include "util.h"

/* ----------------- private helpers ----------------- */

static volatile uint8_t dma_busy = 0; /* is DMA currently pushing data */

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

static inline void SPI_WaitDMA(void) {
    while (dma_busy) {}                          /* wait til DMA finishes */
    while (LL_SPI_IsActiveFlag_BSY(DISP_SPI)) {} /* wait til bytes finished sending */
    LL_SPI_DisableDMAReq_TX(DISP_SPI);
}

static inline void SPI_End(void) {
    while (LL_SPI_IsActiveFlag_BSY(DISP_SPI)) {}    /* wait til bytes finished sending */
    LL_GPIO_SetOutputPin(DISP_PORT_A, DISP_CS_PIN); /* CS high, unlock bus */
}

static inline void DMA_Init(void) {
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2); /* enable DMA2 clock */

    LL_DMA_InitTypeDef DMA_InitStruct = {0};
    DMA_InitStruct.Channel            = DISP_DMA_CHANNEL;
    DMA_InitStruct.Direction          = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    DMA_InitStruct.Mode               = LL_DMA_MODE_NORMAL;

    DMA_InitStruct.PeriphOrM2MSrcAddress = (uint32_t)&(DISP_SPI->DR); /* dest is DR from SPI */
    DMA_InitStruct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT; /* SPI stays in place */

    DMA_InitStruct.MemoryOrM2MDstIncMode  = LL_DMA_MEMORY_INCREMENT; /* src is RAM */
    DMA_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_PDATAALIGN_BYTE;

    DMA_InitStruct.Priority = LL_DMA_PRIORITY_VERYHIGH; /* very high prio to not starve SPI */

    LL_DMA_Init(DISP_DMA, DISP_DMA_STREAM, &DMA_InitStruct);

    LL_DMA_EnableIT_TC(DISP_DMA, DISP_DMA_STREAM); /* enable interrupt on TC */
    NVIC_SetPriority(DMA2_Stream3_IRQn, 0 /* highest priority */);
    NVIC_EnableIRQ(DMA2_Stream3_IRQn);
}

static inline void DMA_Transmit(const uint8_t *buf, uint16_t len) {
    dma_busy = 1;

    LL_DMA_DisableStream(DISP_DMA, DISP_DMA_STREAM); /* disable stream before reconfiguring */
    while (LL_DMA_IsEnabledStream(DISP_DMA, DISP_DMA_STREAM)) {}

    /* clear flags for stream 3 used here */
    LL_DMA_ClearFlag_TC3(DISP_DMA);
    LL_DMA_ClearFlag_HT3(DISP_DMA);
    LL_DMA_ClearFlag_TE3(DISP_DMA);

    LL_DMA_SetMemoryAddress(DISP_DMA, DISP_DMA_STREAM, (uint32_t)buf); /* set up transmitted data */
    LL_DMA_SetDataLength(DISP_DMA, DISP_DMA_STREAM, len);

    LL_DMA_EnableStream(DISP_DMA, DISP_DMA_STREAM);
    LL_SPI_EnableDMAReq_TX(DISP_SPI);
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

    DMA_Init();
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
    LL_mDelay(ST7789V_DELAY_STARTUP); /* VCC pins voltage stabilization */

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

    Display_FillScreen(0x0000);
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

    static uint8_t line_buf[2][ST7789V_DISP_WID * 2]; /* 16bpp double line buf */
    uint8_t        work_idx = 0;                      /* to buf in which CPU currently writes */

    uint8_t color_msb = color >> 8, color_lsb = color & 0xFF;

    Display_SetWindow(x, y, x + wid - 1, y + hei - 1); /* open window of rect size */
    SPI_BeginData();

    for (uint16_t row = 0; row < hei; ++row) {
        for (uint16_t i = 0; i < wid * 2; i += 2)
            line_buf[work_idx][i] = color_msb, line_buf[work_idx][i + 1] = color_lsb;

        /* TODO: add actual scheduler logic here */
        while (dma_busy) {} /* wait for bus free */

        DMA_Transmit(line_buf[work_idx], wid * 2);

        work_idx = !work_idx; /* swap currently worked on buffer */
    }

    /* TODO: add actual scheduler logic here */
    SPI_WaitDMA();
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
    if (c < 32 || c > 126)
        return; /* draw only ASCII chars */

    char tmp_str[2] = {c, '\0'};
    Display_DrawString(x, y, tmp_str, fg_color, bg_color, scale);
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

    uint8_t fg_color_msb = fg_color >> 8, fg_color_lsb = fg_color & 0xFF;
    uint8_t bg_color_msb = bg_color >> 8, bg_color_lsb = bg_color & 0xFF;

    static uint8_t line_buf[2][ST7789V_DISP_WID * 2]; /* 16bpp double line buf */
    uint8_t        work_idx = 0;                      /* to buf in which CPU currently writes */

    Display_SetWindow(x, y, x + total_wid - 1, y + total_hei - 1);
    SPI_BeginData();

    for (uint8_t row = 0; row < 16; ++row)
        for (uint8_t scale_y = 0; scale_y < scale; ++scale_y) {
            if ((row * scale + scale_y) >= total_hei)
                break;

            uint16_t cur_x   = 0;
            uint16_t buf_pos = 0;

            for (uint16_t i = 0; i < len; ++i) {
                uint8_t font_idx = str[i] - ' ';
                uint8_t row_data = terminus_8x16[font_idx][row];

                for (uint8_t col = 0; col < 8; ++col) {
                    uint8_t is_px = row_data & (0x80 >> col);

                    for (uint8_t scale_x = 0; scale_x < scale; ++scale_x) {
                        if (cur_x >= total_wid)
                            break;

                        if (is_px) {
                            line_buf[work_idx][buf_pos++] = fg_color_msb;
                            line_buf[work_idx][buf_pos++] = fg_color_lsb;
                        } else {
                            line_buf[work_idx][buf_pos++] = bg_color_msb;
                            line_buf[work_idx][buf_pos++] = bg_color_lsb;
                        }
                        cur_x++;
                    }
                }
            }

            /* TODO: add actual scheduler logic here */
            while (dma_busy) {} /* wait til it ends sending previous line */

            /* send new line and swap bufs */
            DMA_Transmit(line_buf[work_idx], total_wid * 2);
            work_idx = !work_idx;
        }

    /* TODO: add actual scheduler logic here */
    SPI_WaitDMA();
    SPI_End();
}

void Display_DrawBitmap(uint16_t x, uint16_t y, uint16_t wid, uint16_t hei, const uint8_t *bitmap) {
    if (x + wid > ST7789V_DISP_WID)
        wid = ST7789V_DISP_WID - x;
    if (y + hei > ST7789V_DISP_HEI)
        hei = ST7789V_DISP_HEI - y;

    Display_SetWindow(x, y, x + wid - 1, y + hei - 1);
    SPI_BeginData();

    /* send line by line since its limited by NDTR being 16-bit and a big image would overflow it */
    for (uint16_t row = 0; row < hei; ++row) {
        /* TODO: add actual scheduler logic here */
        while (dma_busy) {} /* wait til it ends sending previous line */

        DMA_Transmit(&bitmap[row * wid * 2], wid * 2);
    }

    /* TODO: add actual scheduler logic here */
    SPI_WaitDMA();
    SPI_End();
}
