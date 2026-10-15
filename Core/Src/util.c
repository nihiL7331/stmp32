#include "util.h"
#include "display.h"
#include <stdio.h>
#include <stdarg.h>

#define BOOTLOG_START_X      10U
#define BOOTLOG_START_Y      10U
#define BOOTLOG_FONT_HEIGHT  16U
#define BOOTLOG_LINE_PADDING 2U /* vertical gap between lines */
#define BOOTLOG_SCREEN_H     ST7789V_DISP_HEI

static uint16_t s_log_y         = BOOTLOG_START_Y;
static uint8_t  s_boot_complete = 0;

void BootLog_Reset(void) {
    STMP32_ASSERT(!s_boot_complete);

    s_log_y = BOOTLOG_START_Y;
    Display_FillScreen(LOG_COL_BG);
}

void BootLog_Print(const char *text, uint16_t fg_col, uint16_t bg_col, uint8_t scale) {
    STMP32_ASSERT(!s_boot_complete);
    STMP32_ASSERT(scale != 0);

    uint16_t line_step = (BOOTLOG_FONT_HEIGHT * scale) + BOOTLOG_LINE_PADDING;

    /* wrap back to top if run out of space */
    if (s_log_y + (BOOTLOG_FONT_HEIGHT * scale) > BOOTLOG_SCREEN_H)
        BootLog_Reset();

    Display_DrawString(BOOTLOG_START_X, s_log_y, text, fg_col, bg_col, scale);
    s_log_y += line_step;

#ifdef DEBUG
    LL_mDelay(150);
#endif /* DEBUG */
}

void BootLog_Printf(uint16_t fg_col, uint16_t bg_col, uint8_t scale, const char *fmt, ...) {
    STMP32_ASSERT(!s_boot_complete);

    char    buf[64];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    BootLog_Print(buf, fg_col, LOG_COL_BG, scale);
}

void BootLog_End(void) {
    s_boot_complete = 1;
    Display_FillScreen(0x0000);
}
