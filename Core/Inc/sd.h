#pragma once

#include <stdint.h>
#include "main.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_utils.h"

/* SPI3 pins */
#define SD_SPI SPI3

/* CS mapping */
#define SD_PORT_CS SD_CS_GPIO_Port
#define SD_CS_PIN  SD_CS_Pin

void    SD_SetSpeed(uint8_t fast);
void    SD_CS_Enable(void);
void    SD_CS_Disable(void);
uint8_t SD_SPI_Transfer(uint8_t data);
