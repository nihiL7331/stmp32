#include "sd.h"

void SD_SetSpeed(uint8_t fast) {
    LL_SPI_Disable(SD_SPI); /* live prescaler changes require disabling SPI temporarily */

    if (fast)
        LL_SPI_SetBaudRatePrescaler(SD_SPI, LL_SPI_BAUDRATEPRESCALER_DIV2);
    else
        LL_SPI_SetBaudRatePrescaler(SD_SPI, LL_SPI_BAUDRATEPRESCALER_DIV256); /* for bw comp */

    LL_SPI_Enable(SD_SPI);
}

void SD_CS_Enable(void) {
    LL_GPIO_ResetOutputPin(SD_PORT_CS, SD_CS_PIN);
}

void SD_CS_Disable(void) {
    LL_GPIO_SetOutputPin(SD_PORT_CS, SD_CS_PIN);
}

uint8_t SD_SPI_Transfer(uint8_t data) {
    uint32_t timeout;

    timeout = 100000;
    while (!LL_SPI_IsActiveFlag_TXE(SD_SPI))
        if (--timeout == 0)
            return 0xFF;
    LL_SPI_TransmitData8(SD_SPI, data);

    /* response byte, 0xFF stands for empty */
    timeout = 100000;
    while (!LL_SPI_IsActiveFlag_RXNE(SD_SPI))
        if (--timeout == 0)
            return 0xFF;
    return LL_SPI_ReceiveData8(SD_SPI);
}
