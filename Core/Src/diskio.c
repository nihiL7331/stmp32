#include "ff.h"
#include "diskio.h"
#include "sd.h"
#include "util.h"

#define SD_RESPONSE_MAX_TRIES 10

/* ----------------- sd commands ----------------- */
#define CMD_BASE 0x40

#define CMD0  (CMD_BASE + 0)  /* GO_IDLE_STATE */
#define CMD2  (CMD_BASE + 2)  /* ALL_SEND_CID */
#define CMD3  (CMD_BASE + 3)  /* SEND_RELATIVE_ADDR */
#define CMD4  (CMD_BASE + 4)  /* SET_DSR */
#define CMD7  (CMD_BASE + 7)  /* SELECT/DESELECT_CARD */
#define CMD8  (CMD_BASE + 8)  /* SEND_IF_COND */
#define CMD9  (CMD_BASE + 9)  /* SEND_CSD */
#define CMD10 (CMD_BASE + 10) /* SEND_CID */
#define CMD11 (CMD_BASE + 11) /* VOLTAGE_SWITCH */
#define CMD12 (CMD_BASE + 12) /* STOP_TRANSMISSION */
#define CMD13 (CMD_BASE + 13) /* SEND_STATUS/SEND_TASK_STATUS */
#define CMD15 (CMD_BASE + 15) /* GO_INACTIVE_STATE */
#define CMD16 (CMD_BASE + 16) /* SET_BLOCKLEN */
#define CMD17 (CMD_BASE + 17) /* READ_SINGLE_BLOCK */
#define CMD18 (CMD_BASE + 18) /* READ_MULTIPLE_BLOCK */
#define CMD19 (CMD_BASE + 19) /* SEND_TUNING_BLOCK */
#define CMD20 (CMD_BASE + 20) /* SPEED_CLASS_CONTROL */
#define CMD22 (CMD_BASE + 22) /* ADDRESS_EXTENSION */
#define CMD23 (CMD_BASE + 23) /* SET_BLOCK_COUNT */
#define CMD24 (CMD_BASE + 24) /* WRITE_BLOCK */
#define CMD25 (CMD_BASE + 25) /* WRITE_MULTIPLE_BLOCK */
#define CMD27 (CMD_BASE + 27) /* PROGRAM_CSD */
#define CMD28 (CMD_BASE + 28) /* SET_WRITE_PROT */
#define CMD29 (CMD_BASE + 29) /* CLR_WRITE_PROT */
#define CMD30 (CMD_BASE + 30) /* SEND_WRITE_PROT */
#define CMD32 (CMD_BASE + 32) /* ERASE_WR_BLK_START */
#define CMD33 (CMD_BASE + 33) /* ERASE_WR_BLK_END */
#define CMD38 (CMD_BASE + 38) /* ERASE */
#define CMD39 (CMD_BASE + 39) /* SELECT_CARD_PARTITION */
#define CMD42 (CMD_BASE + 42) /* LOCK_UNLOCK */
#define CMD43 (CMD_BASE + 43) /* Q_MANAGEMENT */
#define CMD44 (CMD_BASE + 44) /* Q_TASK_INFO_A */
#define CMD45 (CMD_BASE + 45) /* Q_TASK_INFO_B */
#define CMD46 (CMD_BASE + 46) /* Q_RD_TASK */
#define CMD47 (CMD_BASE + 47) /* Q_WR_TASK */
#define CMD48 (CMD_BASE + 48) /* READ_EXTR_SINGLE */
#define CMD49 (CMD_BASE + 49) /* WRITE_EXTR_SINGLE */
#define CMD55 (CMD_BASE + 55) /* APP_CMD */
#define CMD56 (CMD_BASE + 56) /* GEN_CMD */
#define CMD58 (CMD_BASE + 58) /* READ_EXTR_MULTI */
#define CMD59 (CMD_BASE + 59) /* WRITE_EXTR_MULTI */
/* --- I/O mode commands --- */
#define ACMD6  (CMD_BASE + 6)  /* SET_BUS_WIDTH */
#define ACMD13 (CMD_BASE + 13) /* SD_STATUS */
#define ACMD22 (CMD_BASE + 22) /* SEND_NUM_WR_BLOCKS */
#define ACMD23 (CMD_BASE + 23) /* SET_WR_BLK_ERASE_COUNT */
#define ACMD41 (CMD_BASE + 41) /* SD_SEND_OP_COND */
#define ACMD42 (CMD_BASE + 42) /* SET_CLR_CARD_DETECT */
#define ACMD51 (CMD_BASE + 51) /* SEND_SCR */
#define ACMD53 (CMD_BASE + 53) /* SECURE_RECEIVE */
#define ACMD54 (CMD_BASE + 54) /* SECURE_SEND */

/* ----------------- helper functions ----------------- */

static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg) {
    uint8_t n, res;

    if (cmd & 0x80) { /* if ACMD, send CMD55 first */
        cmd &= 0x7F;
        res = SD_SendCmd(CMD55, 0);
        if (res > 1)
            return res;
    }

    /* free line */
    SD_CS_Disable();
    SD_SPI_Transfer(0xFF);
    SD_CS_Enable();
    SD_SPI_Transfer(0xFF);

    /* send cmd bytes */
    SD_SPI_Transfer(cmd);
    SD_SPI_Transfer((uint8_t)(arg >> 24));
    SD_SPI_Transfer((uint8_t)(arg >> 16));
    SD_SPI_Transfer((uint8_t)(arg >> 8));
    SD_SPI_Transfer((uint8_t)(arg & 0xFF));

    /* CRC for CMD0/8 in SPI mode */
    n = 0x01;
    if (cmd == CMD0)
        n = 0x95;
    if (cmd == CMD8)
        n = 0x87;
    SD_SPI_Transfer(n);

    /* wait for response */
    n = SD_RESPONSE_MAX_TRIES;
    do {
        res = SD_SPI_Transfer(0xFF);
    } while ((res & 0x80) && --n);

    return res;
}

/* ----------------- FatFs interface ----------------- */

DSTATUS disk_status(BYTE pdrv) {
    STMP32_ASSERT(pdrv == 0); /* one SD card only (idx0) */
    return 0;
}

DSTATUS disk_initialize(BYTE pdrv) {
    STMP32_ASSERT(pdrv == 0); /* one SD card only (idx0) */

    uint8_t  ocr[4];
    uint16_t tmr;

    SD_SetSpeed(0); /* slow speed for initialization */

    /* "... to execute Fast Boot, host drives CMD line Low for greater than or equal to 74 clocks
     or issues CMD0 with a special argument" */
    SD_CS_Disable();
    for (uint8_t n = 10; n; --n)
        SD_SPI_Transfer(0xFF);

    if (SD_SendCmd(CMD0, 0) == 1)           /* reset card (CMD0) */
        if (SD_SendCmd(CMD8, 0x1AA) == 1) { /* card is idle, check voltage via CMD8 */
            for (uint8_t n = 0; n < 4; ++n)
                ocr[n] = SD_SPI_Transfer(0xFF);

            if (ocr[2] == 0x01 && ocr[3] == 0xAA) { /* supports 2.7-3.6V */
                for (tmr = 1000; tmr; --tmr) {      /* wait for idle exit (ACMD41) */
                    if (SD_SendCmd(ACMD41 | 0x80, 1UL << 30) == 0)
                        break;
                    LL_mDelay(1);
                }

                if (tmr) {
                    SD_SetSpeed(1); /* card initialized, set fast speed */
                    SD_CS_Disable();
                    SD_SPI_Transfer(0xFF);
                    return 0;
                }
            }
        }

    SD_CS_Disable();
    SD_SPI_Transfer(0xFF);
    return STA_NOINIT;
}

/* NOTE: SDHC/SDXC only (block addressing) */
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    STMP32_ASSERT(pdrv == 0); /* one SD card only (idx0) */
    if (count == 0)
        return RES_PARERR;

    while (count--) {
        if (SD_SendCmd(CMD17, sector) != 0) {
            SD_CS_Disable();
            return RES_ERROR;
        }

        uint16_t tmr = 1000;
        while (SD_SPI_Transfer(0xFF) != 0xFE && --tmr) {} /* wait for 0xFE */

        if (tmr == 0) { /* timeout */
            SD_CS_Disable();
            return RES_ERROR;
        }

        for (uint16_t i = 0; i < 512; ++i) /* success, read 512 bytes to RAM buf */
            buff[i] = SD_SPI_Transfer(0xFF);

        SD_SPI_Transfer(0xFF), SD_SPI_Transfer(0xFF); /* skip 2 CRC bytes */

        buff += 512;
        sector++;
    }

    SD_CS_Disable();
    SD_SPI_Transfer(0xFF);

    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    return RES_WRPRT; /* TODO:, until then read-only */
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    return RES_OK;
}

DWORD get_fattime(void) {
    return 0; /* TODO: implement RTC and handle it here */
}
