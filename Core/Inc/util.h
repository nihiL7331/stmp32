#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

/* gcc and clang provide __FILE_NAME__ which is cleaner for output */
/* on a small 2in display, add fallback for other compilers        */
#ifdef __FILE_NAME__
#define SOURCE_FILE __FILE_NAME__
#else /* ifndef __FILE_NAME__ */
#define SOURCE_FILE __FILE__
#endif /* ifndef __FILE_NAME__ */

#ifdef DEBUG
#define STMP32_ASSERT(expr)                                                                        \
    if (!(expr)) {                                                                                 \
        BootLog_Printf(LOG_COL_ERR, LOG_COL_BG, "ASSERT %s:%d", __FILE_NAME__, __LINE__);          \
        __BKPT(0);                                                                                 \
        while (1);                                                                                 \
    }
#else /* !DEBUG */
#define STMP32_ASSERT(expr) ((void)0)
#endif /* !DEBUG */

#define LOG_COL_INFO 0xFFFF /* white  */
#define LOG_COL_OK   0x07E0 /* green  */
#define LOG_COL_WARN 0xFFE0 /* yellow */
#define LOG_COL_ERR  0xF800 /* red    */
#define LOG_COL_BG   0x0000 /* black  */

void BootLog_Reset(void);
void BootLog_Print(const char *text, uint16_t fg_col, uint16_t bg_col, uint8_t scale);
void BootLog_Printf(uint16_t fg_col, uint16_t bg_col, const char *fmt, ...);

#endif /* UTIL_H */
