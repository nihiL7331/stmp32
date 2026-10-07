#pragma once

#ifdef DEBUG
#define STMP32_ASSERT(expr)                                                                        \
    if (!(expr)) {                                                                                 \
        __BKPT(0);                                                                                 \
        while (1)                                                                                  \
            ;                                                                                      \
    }
#else // !DEBUG
#define STMP32_ASSERT(expr) ((void)0)
#endif // !DEBUG
