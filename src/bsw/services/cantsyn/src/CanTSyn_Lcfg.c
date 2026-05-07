/* CanTSyn Link-time Configuration */
#include "CanTSyn.h"
#include "CanTSyn_Cfg.h"

const CanTSyn_TimeDomainConfigType CanTSyn_TimeDomainConfig[CANTSYN_NUMBER_OF_TIME_DOMAINS] = {
    {
        .TimeDomainId = 0,
        .TimeBaseId = 0,
        .IsTimeMaster = TRUE,
        .TxPduId = 0,
        .RxPduId = 1
    }
};
