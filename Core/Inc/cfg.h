#ifndef CFG_H
#define CFG_H

#include <stdint.h>

#define CFG_MAGIC 0xDEADBEEF
#define CFG_VER   1

typedef struct __attribute__((packed)) SystemConfig {
    uint32_t magic; /* expected 0xDEADBEEF */
    uint16_t ver;
    uint8_t  vol;
    char     last_cat[32];
    char     last_track[32];
} SystemCfg;

/* returns a ptr to global cfg state in RAM */
SystemCfg *Cfg_Get(void);

/* loads cfg.bin from SD, if fails/nonexistant, creates a default cfg */
uint8_t Cfg_Init(void);

/* flushes curr RAM state to cfg.bin on SD */
uint8_t Cfg_Save(void);

#endif /* CFG_H */
