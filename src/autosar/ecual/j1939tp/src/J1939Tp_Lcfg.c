/* J1939Tp Link-time Configuration */
#include "J1939Tp.h"
#include "J1939Tp_Cfg.h"

const J1939Tp_NSduConfigType J1939Tp_NSduConfig[J1939TP_NUM_NSDUS] = {
    {
        .NSduId = 0,
        .ConnectionIdx = 0,
        .Protocol = J1939TP_PROTOCOL_BAM,
        .TxPduId = 0,
        .RxPduId = 1
    }
};
