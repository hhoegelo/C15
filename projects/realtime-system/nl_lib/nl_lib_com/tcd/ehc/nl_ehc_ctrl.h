/******************************************************************************/
/** @file		nl_ehc_ctrl.h
    @date		2020-01-10
    @version	0
    @author		KSTR
    @brief		handlers for external controllers (pedals etc) processing
    @ingroup	nl_tcd_modules
*******************************************************************************/

#ifndef NL_EHC_CTRL_H_
#define NL_EHC_CTRL_H_

#include <stdint.h>

void NL_EHC_InitControllers(void);
void NL_EHC_ProcessControllers(void);
void NL_EHC_SetLegacyPedalType(const uint16_t controller, uint16_t type);
void NL_EHC_SetEHCconfig(const uint16_t cmd, uint16_t data);
void NL_EHC_RequestToSendEHCdata(void);

#endif
//EOF
