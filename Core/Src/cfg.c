#include "cfg.h"
#include "ff.h"
#include "util.h"
#include <string.h>

static SystemCfg   g_cfg;
static const char *CFG_FILENAME = "cfg.bin";

/* -------------------- helpers -------------------- */

static void Cfg_SetDefaults(void) {
    memset(&g_cfg, 0, sizeof(SystemCfg));
    g_cfg.magic = CFG_MAGIC;
    g_cfg.ver   = CFG_VER;
    g_cfg.vol   = 50;
}

/* -------------------- api impl -------------------- */

SystemCfg *Cfg_Get(void) {
    return &g_cfg;
}

uint8_t Cfg_Init(void) {
    FIL     fil;
    UINT    br;
    FRESULT fr;

    BootLog_Printf(LOG_COL_INFO, LOG_COL_BG, 1, "loading cfg.bin...");

    fr = f_open(&fil, CFG_FILENAME, FA_READ);
    if (fr == FR_OK) {
        fr = f_read(&fil, &g_cfg, sizeof(SystemCfg), &br);
        f_close(&fil);

        if (fr == FR_OK && br == sizeof(SystemCfg) && g_cfg.magic == CFG_MAGIC) {
            BootLog_Printf(LOG_COL_OK, LOG_COL_BG, 1, "cfg OK (vol: %d)", g_cfg.vol);
            return 1;
        } else
            BootLog_Printf(LOG_COL_WARN, LOG_COL_BG, 1, "cfg invalid, fallback to def");
    } else
        BootLog_Printf(LOG_COL_WARN, LOG_COL_BG, 1, "cfg not found, fallback to def");

    Cfg_SetDefaults(); /* this gets called if file didnt exist/read failed/magic was corrupt */

    if (Cfg_Save()) {
        BootLog_Printf(LOG_COL_OK, LOG_COL_BG, 1, "cfg def created");
        return 1;
    } else {
        BootLog_Printf(LOG_COL_ERR, LOG_COL_BG, 1, "cfg create fail");
        return 0;
    }
}

uint8_t Cfg_Save(void) {
    FIL     fil;
    UINT    bw;
    FRESULT fr;

    fr = f_open(&fil, CFG_FILENAME, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK)
        return 0;

    fr = f_write(&fil, &g_cfg, sizeof(SystemCfg), &bw);
    f_close(&fil);

    return (fr == FR_OK && bw == sizeof(SystemCfg));
}
