#pragma once

#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_utils.h"
#include "stm32f4xx_ll_dma.h"
#include <stdint.h>

/* SPI1 */
#define DISP_SPI          SPI1
#define DISP_SPI_CLK_EN() LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1)

/* A CLK AHB1, SCK, MOSI, CS */
#define DISP_PORT_A          GPIOA
#define DISP_PORT_A_CLK_EN() LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA)
#define DISP_SCK_PIN         LL_GPIO_PIN_5
#define DISP_MOSI_PIN        LL_GPIO_PIN_7
#define DISP_CS_PIN          LL_GPIO_PIN_4

/* B CLK AHB1, RST, DC */
#define DISP_PORT_B          GPIOB
#define DISP_PORT_B_CLK_EN() LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB)
#define DISP_RST_PIN         LL_GPIO_PIN_0
#define DISP_DC_PIN          LL_GPIO_PIN_1

/* DMA config */
#define DISP_DMA         DMA2
#define DISP_DMA_STREAM  LL_DMA_STREAM_3
#define DISP_DMA_CHANNEL LL_DMA_CHANNEL_3

/* ST7789V hardware commands */
#define ST7789V_CMD_NOP       0x00
#define ST7789V_CMD_SWRESET   0x01 /* Software Reset */
#define ST7789V_CMD_RDDID     0x04 /* Read Display ID */
#define ST7789V_CMD_RDDST     0x09 /* Read Display Status */
#define ST7789V_CMD_RDDPM     0x0A /* Read Display Power Mode */
#define ST7789V_CMD_RDDMADCTL 0x0B /* Read Display MADCTL */
#define ST7789V_CMD_RDDCOLMOD 0x0C /* Read Display Pixel Format */
#define ST7789V_CMD_RDDIM     0x0D /* Read Display Image Mode */
#define ST7789V_CMD_RDDSM     0x0E /* Read Display Signal Mode */
#define ST7789V_CMD_RDDSDR    0x0F /* Read Display Self-Diagnostic Result */
#define ST7789V_CMD_SLPIN     0x10 /* Sleep in */
#define ST7789V_CMD_SLPOUT    0x11 /* Sleep Out */
#define ST7789V_CMD_PTLON     0x12 /* Partial Display Mode On */
#define ST7789V_CMD_NORON     0x13 /* Normal Display Mode On */
#define ST7789V_CMD_INVOFF    0x20 /* Display Inversion Off */
#define ST7789V_CMD_INVON     0x21 /* Display Inversion On */
#define ST7789V_CMD_GAMSET    0x26 /* Gamma Set */
#define ST7789V_CMD_DISPOFF   0x28 /* Display Off */
#define ST7789V_CMD_DISPON    0x29 /* Display On */
#define ST7789V_CMD_CASET     0x2A /* Column Address Set */
#define ST7789V_CMD_RASET     0x2B /* Row Address Set */
#define ST7789V_CMD_RAMWR     0x2C /* Memory Write */
#define ST7789V_CMD_RAMRD     0x2E /* Memory Read */
#define ST7789V_CMD_PTLAR     0x30 /* Partial Area */
#define ST7789V_CMD_VSCRDEF   0x33 /* Vertical Scrolling Definition */
#define ST7789V_CMD_TEOFF     0x34 /* Tearing Effect Line Off */
#define ST7789V_CMD_TEON      0x35 /* Tearing Effect Line On */
#define ST7789V_CMD_MADCTL    0x36 /* Memory Data Access Control */
#define ST7789V_CMD_VSCSAD    0x37 /* Vertical Scroll Start Address of RAM */
#define ST7789V_CMD_IDMOFF    0x38 /* Idle Mode Off */
#define ST7789V_CMD_IDMON     0x39 /* Idle Mode On */
#define ST7789V_CMD_COLMOD    0x3A /* Interface Pixel Format */
#define ST7789V_CMD_WRMEMC    0x3C /* Write Memory Continue */
#define ST7789V_CMD_RDMEMC    0x3E /* Read Memory Continue */
#define ST7789V_CMD_STE       0x44 /* Set Tear Scanline */
#define ST7789V_CMD_GSCAN     0x45 /* Get Scanline */
#define ST7789V_CMD_WRDISBV   0x51 /* Write Display Brightness */
#define ST7789V_CMD_RDDISBV   0x52 /* Read Display Brightness Value */
#define ST7789V_CMD_WRCTRLD   0x53 /* Write CTRL Display */
#define ST7789V_CMD_RDCTRLD   0x54 /* Read CTRL Value Display */
#define ST7789V_CMD_WRCACE    0x55 /* Write Content Adaptive Brightness */
#define ST7789V_CMD_RDCABC    0x56 /* Read Content Adaptive Brightness */
#define ST7789V_CMD_WRCABCMB  0x5E /* Write CABC Minimum Brightness */
#define ST7789V_CMD_RDCABCMB  0x5F /* Read CABC Minimum Brightness */
#define ST7789V_CMD_RDABCSDR  0x68 /* Read Automatic Brightness Control Self-Diagnostic Result */
#define ST7789V_CMD_RAMCTRL   0xB0 /* RAM Control */
#define ST7789V_CMD_RGBCTRL   0xB1 /* RGB Interface Control */
#define ST7789V_CMD_PORCTRL   0xB2 /* Porch Setting */
#define ST7789V_CMD_FRCTRL1   0xB3 /* Frame Rate Control 1 in Partial Mode/Idle Colors */
#define ST7789V_CMD_PARCTRL   0xB5 /* Partial Mode Control */
#define ST7789V_CMD_GCTRL     0xB7 /* Gate Control */
#define ST7789V_CMD_GTADJ     0xB8 /* Gate On Timing Adjustment */
#define ST7789V_CMD_DGMEN     0xBA /* Digital Gamma Enable */
#define ST7789V_CMD_VCOMS     0xBB /* VCOMS Setting */
#define ST7789V_CMD_LCMCTRL   0xC0 /* LCM Control */
#define ST7789V_CMD_IDSET     0xC1 /* ID Code Setting */
#define ST7789V_CMD_VDVVRHEN  0xC2 /* VDV and VRH Command Enable */
#define ST7789V_CMD_VRHS      0xC3 /* VRH Set */
#define ST7789V_CMD_VDVS      0xC4 /* VDV Set */
#define ST7789V_CMD_VCMOFSET  0xC5 /* VCOMS Offset Set */
#define ST7789V_CMD_FRCTRL2   0xC6 /* Frame Rate Control in Normal Mode */
#define ST7789V_CMD_CABCCTRL  0xC7 /* CABC Control */
#define ST7789V_CMD_REGSEL1   0xC8 /* Register Value Selection 1 */
#define ST7789V_CMD_REGSEL2   0xCA /* Register Value Selection 2 */
#define ST7789V_CMD_PWMFRSEL  0xCC /* PWM Frequency Selection */
#define ST7789V_CMD_PWCTRL1   0xD0 /* Power Control 1 */
#define ST7789V_CMD_VAPVANEN  0xD2 /* Enable VAP/VAN Signal Output */
#define ST7789V_CMD_RDID1     0xDA /* Read ID1 */
#define ST7789V_CMD_RDID2     0xDB /* Read ID2 */
#define ST7789V_CMD_RDID3     0xDC /* Read ID3 */
#define ST7789V_CMD_CMD2EN    0xDF /* Command 2 Enable */
#define ST7789V_CMD_PVGAMCTRL 0xE0 /* Positive Voltage Gamma Control */
#define ST7789V_CMD_NVGAMCTRL 0xE1 /* Negative Voltage Gamma Control */
#define ST7789V_CMD_DMGLUTR   0xE2 /* Digital Gamma Look-up Table for Red */
#define ST7789V_CMD_DGMLUTB   0xE3 /* Digital Gamma Look-up Table for Blue */
#define ST7789V_CMD_GATECTRL  0xE4 /* Gate Control */
#define ST7789V_CMD_SPI2EN    0xE7 /* SPI2 Enable */
#define ST7789V_CMD_PWCTRL2   0xE8 /* Power Control 2 */
#define ST7789V_CMD_EQCTRL    0xE9 /* Equalize Time Control */
#define ST7789V_CMD_PROMCTRL  0xEC /* Program Mode Control */
#define ST7789V_CMD_PROMEN    0xFA /* Program Mode Enable */
#define ST7789V_CMD_NVMSET    0xFC /* NVM Setting */
#define ST7789V_CMD_PROMACT   0xFE /* Program Action */

/* ST7789V delays (in ms) */
#define ST7789V_DELAY_HWRESET 50  /* after hardware reset */
#define ST7789V_DELAY_SWRESET 150 /* after software reset */
#define ST7789V_DELAY_SLPOUT  120 /* after Sleep Out command */
#define ST7789V_DELAY_STATE   10  /* after enabling display */

/* ST7789V display (in px) */
#define ST7789V_DISP_WID 240
#define ST7789V_DISP_HEI 320

/* ST7789V color mode */
#define ST7789V_COLMOD_12BIT 0x03
#define ST7789V_COLMOD_16BIT 0x55
#define ST7789V_COLMOD_18BIT 0x66

/* ST7789V MADCTL opts */
#define ST7789V_MADCTL_MY  0x80 /* Row Address Order, flip Y */
#define ST7789V_MADCTL_MX  0x40 /* Column Address Order, flip X */
#define ST7789V_MADCTL_MV  0x20 /* Row/Column Exchange, swap X/Y */
#define ST7789V_MADCTL_ML  0x10 /* Scan Address Order */
#define ST7789V_MADCTL_RGB 0x00 /* RGB Order */
#define ST7789V_MADCTL_BGR 0x08 /* BGR Order */
#define ST7789V_MADCTL_MH  0x04 /* Horizontal Order */

void Display_InitHardware(void);
void Display_SendCommand(uint8_t cmd);
void Display_SendData(uint8_t data);
void Display_Init(void);
void Display_SetWindow(uint16_t start_x, uint16_t start_y, uint16_t end_x, uint16_t end_y);
void Display_FillRect(uint16_t x, uint16_t y, uint16_t wid, uint16_t hei, uint16_t color);
void Display_FillScreen(uint16_t color);
void Display_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void Display_DrawChar(uint16_t x, uint16_t y, char c, uint16_t fg_color, uint16_t bg_color,
                      uint8_t scale);
void Display_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t fg_color,
                        uint16_t bg_color, uint8_t scale);
