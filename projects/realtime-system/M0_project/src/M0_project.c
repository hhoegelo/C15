/******************************************************************************/
/**	@file 	M0_IPC_Test.c
	@date	2016-03-09 SSC
  	@author	2015-01-31 DTZ

	@brief	see the M4_IPC_Test.c file for the documentation

  	@note	!!!!!! USE optimized most -O3 for compiling !!!!!!

*******************************************************************************/
#include <stdint.h>
#include "cmsis/LPC43xx.h"
#include "boards/emphase_v5.h"
#include "ipc/emphase_ipc.h"
#include "drv/nl_rit.h"
#include "drv/nl_cgu.h"
#include "drv/nl_gpdma.h"
#include "drv/nl_kbs.h"
#include "drv/nl_dbg.h"
#include "espi/nl_espi_core.h"
#include "espi/dev/nl_espi_dev_aftertouch.h"
#include "espi/dev/nl_espi_dev_pedals.h"
#include "espi/dev/nl_espi_dev_pitchbender.h"
#include "espi/dev/nl_espi_dev_ribbons.h"
#include "espi/dev/nl_espi_dev_adc.h"

#define ESPI_MODE_ADC      LPC_SSP0, ESPI_CPOL_0 | ESPI_CPHA_0
#define ESPI_MODE_ATT_DOUT LPC_SSP0, ESPI_CPOL_0 | ESPI_CPHA_0
#define ESPI_MODE_DIN      LPC_SSP0, ESPI_CPOL_1 | ESPI_CPHA_1

static inline uint32_t M0TicksToUS(uint32_t const ticks)
{
  return (M0_SYSTICK_IN_NS * (ticks >> IPC_KEYBUFFER_TIME_SHIFT) + 499) / 1000;  // rounded
}

/******************************************************************************/
/** @note	Espi device functions do NOT switch mode themselves!
 	 	 	espi bus speed: 1.6 MHz -> 0.625 µs                    Bytes    t_µs
                                                     POL/PHA | multi | t_µs_sum
--------------------------------------------------------------------------------
P1D1   ADC-2CH     ribbon_board         MCP3202   R/W  0/1   3   2   15  30
P1D2   ADC-1CH     pitch_bender_board   MCP3201   R    0/1   3   1   15  15
--------------------------------------------------------------------------------
P0D1   1CH-ADC     aftertouch_board     MCP3201   R    0/1   3   1   15  15
--------------------------------------------------------------------------------
P2D2   ATTENUATOR  pedal_audio_board    LM1972    W    0/0   2   2   10  20
--------------------------------------------------------------------------------
P3D1   ADC-1CH     volume_poti_board    MCP3201   R    0/1   3   1   15  15
--------------------------------------------------------------------------------
P4D1   ADC-8CH     pedal_audio_board    MCP3208   R/W  0/1   3   4   15  60
P4D2   DETECT      pedal_audio_board    HC165     R    1/1   1   1    5   5
P4D3   SET_PULL_R  pedal_audio_board    HC595     W    0/0   1   1    5   5
--------------------------------------------------------------------------------
                                                                13      165
*******************************************************************************/
static void ProcessADCs(void)
{
  static uint32_t compensatingTicks = 0;
  static uint32_t totalTicks        = 0;
  static uint32_t hbLedCnt          = 0;
  static uint8_t  first             = 1;
  int32_t         val;
  uint32_t        savedTicks;

  if (first)
  {
    first      = 0;
    totalTicks = s.timer;
  }
  else
  {
    IPC_SetADCTime(savedTicks = (s.timer - totalTicks));
    totalTicks = s.timer;
    // Feedback loop to adjust the cycle time to collect all the ADC data 16 times
    // per 12.5ms ADC cycle (the feedback is slow, only +- 1 tick, ~1us, of adjustment
    // per run of this process routine).
    // This is because the ADC readout from M4 side is with 16-fold averaging
    // which assumes we have 16 values collected during the 12.5ms.
    // The feedback loop is protected from trying to make the cycle time shorter
    // than physically possible, determined by interrupt load. This is the standard
    // case at the moment as ProcessADCs() execution time is just slightly longer
    // than the 781us required.
    while (s.timer - totalTicks < compensatingTicks)
      ;
    if (M0TicksToUS(savedTicks) < 12500 / 16)  // cycle was faster than 16 rounds per 12.5ms ?
      compensatingTicks += (1 << IPC_KEYBUFFER_TIME_SHIFT);
    else
    {
      if (compensatingTicks)  // cycle time can be shortened ?
        compensatingTicks -= (1 << IPC_KEYBUFFER_TIME_SHIFT);
    }
  }

  // switch mode: 13.6 µs
  // now, all adc channel & detect values have been read ==> sync read index to write index
  IPC_AdcUpdateReadIndex();
  // Starting a new round of adc channel & detect value read-ins, advance ipc write index first
  IPC_AdcBufferWriteNext();

  SPI_DMA_SwitchMode(ESPI_MODE_ADC);
  NL_GPDMA_Poll();

  // do heartbeat LED here, enough time
  hbLedCnt++;
  switch (hbLedCnt)
  {
    case 1:
      DBG_Led_Cpu_On();
      break;
    case 500:
      DBG_Led_Cpu_Off();
      break;
    case 1000:
      hbLedCnt = 0;
      break;
    default:
      break;
  }
  // pedal 1 : 57 µs
  ESPI_DEV_Pedals_EspiPullChannel_Blocking(ESPI_PEDAL_1_ADC_RING);
  NL_GPDMA_Poll();
  val = ESPI_DEV_Pedals_GetValue(ESPI_PEDAL_1_ADC_RING);
  IPC_WriteAdcBuffer(IPC_ADC_PEDAL1_RING, val);

  ESPI_DEV_Pedals_EspiPullChannel_Blocking(ESPI_PEDAL_1_ADC_TIP);
  NL_GPDMA_Poll();
  val = ESPI_DEV_Pedals_GetValue(ESPI_PEDAL_1_ADC_TIP);
  IPC_WriteAdcBuffer(IPC_ADC_PEDAL1_TIP, val);

  // pedal 2 : 57 µs
  ESPI_DEV_Pedals_EspiPullChannel_Blocking(ESPI_PEDAL_2_ADC_RING);
  NL_GPDMA_Poll();
  val = ESPI_DEV_Pedals_GetValue(ESPI_PEDAL_2_ADC_RING);
  IPC_WriteAdcBuffer(IPC_ADC_PEDAL2_RING, val);

  ESPI_DEV_Pedals_EspiPullChannel_Blocking(ESPI_PEDAL_2_ADC_TIP);
  NL_GPDMA_Poll();
  val = ESPI_DEV_Pedals_GetValue(ESPI_PEDAL_2_ADC_TIP);
  IPC_WriteAdcBuffer(IPC_ADC_PEDAL2_TIP, val);

  // pedal 3 : 57 µs
  ESPI_DEV_Pedals_EspiPullChannel_Blocking(ESPI_PEDAL_3_ADC_RING);
  NL_GPDMA_Poll();
  val = ESPI_DEV_Pedals_GetValue(ESPI_PEDAL_3_ADC_RING);
  IPC_WriteAdcBuffer(IPC_ADC_PEDAL3_RING, val);

  ESPI_DEV_Pedals_EspiPullChannel_Blocking(ESPI_PEDAL_3_ADC_TIP);
  NL_GPDMA_Poll();
  val = ESPI_DEV_Pedals_GetValue(ESPI_PEDAL_3_ADC_TIP);
  IPC_WriteAdcBuffer(IPC_ADC_PEDAL3_TIP, val);

  // pedal 4 : 57 µs
  ESPI_DEV_Pedals_EspiPullChannel_Blocking(ESPI_PEDAL_4_ADC_RING);
  NL_GPDMA_Poll();
  val = ESPI_DEV_Pedals_GetValue(ESPI_PEDAL_4_ADC_RING);
  IPC_WriteAdcBuffer(IPC_ADC_PEDAL4_RING, val);

  ESPI_DEV_Pedals_EspiPullChannel_Blocking(ESPI_PEDAL_4_ADC_TIP);
  NL_GPDMA_Poll();
  val = ESPI_DEV_Pedals_GetValue(ESPI_PEDAL_4_ADC_TIP);
  IPC_WriteAdcBuffer(IPC_ADC_PEDAL4_TIP, val);

  // detect pedals: 32.5 µs
  SPI_DMA_SwitchMode(ESPI_MODE_DIN);
  NL_GPDMA_Poll();

  ESPI_DEV_Pedals_Detect_EspiPull();
  NL_GPDMA_Poll();

  uint8_t detect = ESPI_DEV_Pedals_Detect_GetValue();
  IPC_WriteAdcBuffer(IPC_ADC_PEDAL4_DETECT, ((detect & 0b00010000) >> 4) ? 4095 : 0);
  IPC_WriteAdcBuffer(IPC_ADC_PEDAL3_DETECT, ((detect & 0b00100000) >> 5) ? 4095 : 0);
  IPC_WriteAdcBuffer(IPC_ADC_PEDAL2_DETECT, ((detect & 0b01000000) >> 6) ? 4095 : 0);
  IPC_WriteAdcBuffer(IPC_ADC_PEDAL1_DETECT, ((detect & 0b10000000) >> 7) ? 4095 : 0);

  // set pull resistors: best case: 17.3 µs - worst case: 36 µs
  SPI_DMA_SwitchMode(ESPI_MODE_ATT_DOUT);
  NL_GPDMA_Poll();

  uint32_t config = IPC_ReadPedalAdcConfig();
  ESPI_DEV_Pedals_SetPedalState(1, (uint8_t)((config >> 0) & 0xFF));
  ESPI_DEV_Pedals_SetPedalState(2, (uint8_t)((config >> 8) & 0xFF));
  ESPI_DEV_Pedals_SetPedalState(3, (uint8_t)((config >> 16) & 0xFF));
  ESPI_DEV_Pedals_SetPedalState(4, (uint8_t)((config >> 24) & 0xFF));

  ESPI_DEV_Pedals_PullResistors_EspiSendIfChanged();

  NL_GPDMA_Poll();

  // pitchbender: 42 µs
  SPI_DMA_SwitchMode(ESPI_MODE_ADC);
  NL_GPDMA_Poll();

  ESPI_DEV_Pitchbender_EspiPull();
  NL_GPDMA_Poll();

  IPC_WriteAdcBuffer(IPC_ADC_PITCHBENDER, ESPI_DEV_Pitchbender_GetValue());

  // aftertouch: 29 µs
  ESPI_DEV_Aftertouch_EspiPull();
  NL_GPDMA_Poll();

  IPC_WriteAdcBuffer(IPC_ADC_AFTERTOUCH, ESPI_DEV_Aftertouch_GetValue());

  // 2 ribbons: 57 µs
  ESPI_DEV_Ribbons_EspiPull_Upper();
  NL_GPDMA_Poll();
  IPC_WriteAdcBuffer(IPC_ADC_RIBBON1, ESPI_DEV_Ribbons_GetValue(UPPER_RIBBON));

  ESPI_DEV_Ribbons_EspiPull_Lower();
  NL_GPDMA_Poll();
  IPC_WriteAdcBuffer(IPC_ADC_RIBBON2, ESPI_DEV_Ribbons_GetValue(LOWER_RIBBON));
}

/******************************************************************************/
int main(void)
{
  EMPHASE_V5_M0_Init();

  Emphase_IPC_Init();

  NL_GPDMA_Init(0b00000011);  // inverse to the mask in the M4_Main

  // 20MHz clock freq... works up to ~50MHz in V7.1 hardware (AT not checked, though).
  // With 20MHz, max M0 cycle time seems to be just under 90us even worst case, so
  // overrun (when > 125us) is unlikely, the scheduling always stays in sync, no missed
  // time slices
  ESPI_Init(2000000);

  ESPI_DEV_Pedals_Init();
  ESPI_DEV_Aftertouch_Init();
  ESPI_DEV_Pitchbender_Init();
  ESPI_DEV_Ribbons_Init();

  RIT_Init_IntervalInNs(M0_SYSTICK_IN_NS);

  while (1)
    ProcessADCs();

  return 0;
}

/******************************************************************************/
static inline void SendInterruptToM4(void)
{
  __DSB();
  __SEV();
}

/******************************************************************************/

void M0_RIT_OR_WWDT_IRQHandler(void)
{
  // Clear interrupt flag early, to allow for short, single cycle IRQ overruns,
  // that is if, the keybed scanner occasionally takes longer than one time slot
  // (but no more than two), the interrupt routine is invoked again as soon as
  // it completed the first pass. By this, no IRQ is lost and M4 clocking precision
  // is guaranteed, albeit with an occasional jitter.
  // Only when eeprom memory access etc slows down the bus there is a slight
  // chance of a burst of overruns where the M0 main process hardly gets any execution
  // time and the clocking of the M4 core may slew for that time.
  RIT_ClearInt();

  s.timer += (1 << IPC_KEYBUFFER_TIME_SHIFT);

  if (!(s.timer & (M0_SYSTICKS_PER_PERIOD_MASK << IPC_KEYBUFFER_TIME_SHIFT)))
    SendInterruptToM4();

  (*KBS_Process)();

  // s.RitCrtlReg |= RIT_GetCtrlReg();
}
