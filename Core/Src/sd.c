#include "sd.h"
#include "util.h"

#define SD_SPI_FLAG_TIMEOUT 100000U
#define SD_SPI_EMPTY_BYTE   0xFF

void SD_SetSpeed(uint8_t fast) {
    STMP32_ASSERT(fast == SD_SPEED_FAST || fast == SD_SPEED_SLOW);

    LL_SPI_Disable(SD_SPI); /* live prescaler changes require disabling SPI temporarily */

    if (fast == SD_SPEED_FAST)
        LL_SPI_SetBaudRatePrescaler(SD_SPI, LL_SPI_BAUDRATEPRESCALER_DIV2);
    else /* if fast == SD_SPEED_SLOW */
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
    uint32_t timeout = SD_SPI_FLAG_TIMEOUT;
    while (!LL_SPI_IsActiveFlag_TXE(SD_SPI))
        if (--timeout == 0)
            return SD_SPI_EMPTY_BYTE;
    LL_SPI_TransmitData8(SD_SPI, data);

    /* response byte, empty stands for 0xFF */
    timeout = SD_SPI_FLAG_TIMEOUT;
    while (!LL_SPI_IsActiveFlag_RXNE(SD_SPI))
        if (--timeout == 0)
            return SD_SPI_EMPTY_BYTE;
    return LL_SPI_ReceiveData8(SD_SPI);
}
