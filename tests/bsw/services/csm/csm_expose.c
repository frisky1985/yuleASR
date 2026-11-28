/**
 * @file csm_expose.c
 * @brief Expose the static internals of Csm.c to the unit tests.
 *
 * Several Csm helpers (queue insert/dequeue/process, job execution, key
 * element load, KDF append helpers) are file-scope (static) and some have no
 * caller at all in this tree, so they cannot be reached through the public
 * API. Re-defining `static` away for this single translation unit makes them
 * global so the tests can call them directly and drive every branch. This is
 * the same technique used by fee_expose.c / wdg_hw_expose.c.
 *
 * System headers are pulled in first so `#define static` only affects the
 * statics declared inside Csm.c itself.
 */

#include <string.h>
#include <stdint.h>

#define static
#include "../../../../src/bsw/services/csm/src/Csm.c"
#undef static

/*---- 测试桥接: 强制把指定 key 的元素 0 置为有效 ----
 * 公开 API 的 KeyElementSet 在该实现中会追加到新元素槽 (元素0 常为占位),
 * 而派生/交换的软件回退固定读取 elements[0]，故通过本桥接直接构造前置状态，
 * 以驱动 Csm_KeyDeriveSw / Csm_KeyExchange*Sw 等分支。 */
void CsmTest_ForceKeyElement(uint32 keyId, const uint8* data, uint32 length)
{
    uint8 idx;
    if ((data != NULL_PTR) && (length <= CSM_MAX_KEY_LENGTH) &&
        (Csm_FindKeyIndex(keyId, &idx) == E_OK))
    {
        if (Csm_Keys[idx].numElements == 0U)
        {
            Csm_Keys[idx].numElements = 1U;
        }
        (void)Mcal_MemCopy(Csm_Keys[idx].elements[0].data, data, length);
        Csm_Keys[idx].elements[0].length = length;
        Csm_Keys[idx].elements[0].valid = TRUE;
        Csm_UpdateKeyStatus(idx, CSM_KEY_STATUS_VALID);
    }
}
