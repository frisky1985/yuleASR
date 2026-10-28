/**
 * @file SchM_DoIP.h
 * @brief SchM stub for DoIP unit tests (host only).
 *
 * The real SchM_DoIP.h maps SchM_Enter/Exit_DoIP() onto Mcal interrupt
 * control functions; on the host they are no-ops with a capture counter.
 */

#ifndef SCHM_DOIP_STUB_H
#define SCHM_DOIP_STUB_H

extern void test_DoipEnterCs(void);
extern void test_DoipExitCs(void);

#ifndef SchM_Enter_DoIP
#define SchM_Enter_DoIP()   test_DoipEnterCs()
#endif

#ifndef SchM_Exit_DoIP
#define SchM_Exit_DoIP()    test_DoipExitCs()
#endif

#endif /* SCHM_DOIP_STUB_H */
