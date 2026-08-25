#include "Uart_Cfg.h"

void UnityOutputChar(char c)
{
    if (c == '\n')
    {
        Uart_WriteByte('\r');
    }
    Uart_WriteByte((uint8_t)c);
}

void UnityOutputFlush(void)
{
}
