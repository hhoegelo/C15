/******************************************************************************/
/** @file		nl_ehc.c
    @date		2020-01-15
    @version	0
    @author		KSTR
    @brief		External Hardware Controllers (pedals etc) processing
    @ingroup	nl_tcd_modules
*******************************************************************************/

#include "nl_ehc.h"
#include "nl_ehc_adc.h"
#include "nl_ehc_ctrl.h"
#include "drv/nl_dbg.h"

/*************************************************************************/ /**
* @brief	init everything
******************************************************************************/
void NL_EHC_Init(void)
{
  initSampleBuffers();
  NL_EHC_InitControllers();
}

/*************************************************************************/ /**
* @brief	de-init everything
******************************************************************************/
void NL_EHC_DeInit(void)
{
  NL_EHC_DeInitControllers();
}

/*************************************************************************/ /**
* @brief	main process
* main repetitive process called from ADC_Work_Process
******************************************************************************/
void NL_EHC_Process(void)
{
  // any processing only after buffers are fully filled initially, also gives some initial power-up settling
  if (!FillSampleBuffers())
    return;

  //  DBG_GPIO3_1_On();
  NL_EHC_ProcessControllers();
  //  DBG_GPIO3_1_Off();
}

// EOF
