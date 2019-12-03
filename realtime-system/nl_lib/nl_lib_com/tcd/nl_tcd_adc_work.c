/*
 * nl_tcd_adc_work.c
 *
 *  Created on: 13.03.2015
 *      Author: ssc
 */

#include "math.h"

#include "nl_tcd_adc_work.h"
#include "espi/dev/nl_espi_dev_pedals.h"
#include "ipc/emphase_ipc.h"

#include "ipc/emphase_ipc.h"
#include "nl_tcd_param_work.h"
#include "spibb/nl_bb_msg.h"

#include "nl_tcd_test.h"
#include "nl_tcd_interpol.h"

#define GROUND_THRESHOLD     20
#define CHANGE_FOR_DETECTION 500

#if 0
#define RIBBON_THRESHOLD (100)
#define RIBBON_MIN       (160)
#define RIBBON_MAX       (4040)
#define RIBBON_SPAN      (RIBBON_MAX - RIBBON_MIN)

#define RIBBON_CORR_4000  50   // 0 ... 128 refers to 0 ... 3.125 %, upscaled by 1024
#define RIBBON_CORR_8000  120  // 0 ... 128 refers to 0 ... 3.125 %, upscaled by 1024
#define RIBBON_CORR_12000 110  // 0 ... 128 refers to 0 ... 3.125 %, upscaled by 1024
#endif

#define DELTA 40  // hysteresis for pedals

#define BENDER_DEADRANGE    20    // +/-1 % of +/-2047
#define BENDER_SMALL_THRESH 200   // +/-10 % of +/-2047, test range
#define BENDER_TEST_PERIOD  80    // 1 s = 80 * 12.5 ms
#define BENDER_RAMP_INC     8     // 400 / 8 = 50 steps from the threshold to zero (50 * 12.5 ms = 625 ms)
#define BENDER_FACTOR       4540  // 4540 / 4096 = 2047 / 1847 for saturation = 100 % at 90 % of the input range

#define AT_DEADRANGE 30    // 0.73 % of 0 ... 4095
#define AT_FACTOR    5080  // 5080 / 4096 for saturation = 100 % at 81 % of the input range

#define NUM_HW_SOURCES 8

#define HW_SOURCE_ID_PEDAL_1    0
#define HW_SOURCE_ID_PEDAL_2    1
#define HW_SOURCE_ID_PEDAL_3    2
#define HW_SOURCE_ID_PEDAL_4    3
#define HW_SOURCE_ID_PITCHBEND  4
#define HW_SOURCE_ID_AFTERTOUCH 5
#define HW_SOURCE_ID_RIBBON_1   6
#define HW_SOURCE_ID_RIBBON_2   7

static uint32_t bbSendValue[NUM_HW_SOURCES] = {};

static uint32_t hwParamId[NUM_HW_SOURCES] = { PARAM_ID_PEDAL_1,
                                              PARAM_ID_PEDAL_2,
                                              PARAM_ID_PEDAL_3,
                                              PARAM_ID_PEDAL_4,
                                              PARAM_ID_PITCHBEND,
                                              PARAM_ID_AFTERTOUCH,
                                              PARAM_ID_RIBBON_1,
                                              PARAM_ID_RIBBON_2 };

static uint32_t pedalDetected[4];
static uint32_t tipPullup[4];
static uint32_t inverted[4];
static uint32_t tipActive[4];

static uint32_t checkPedal[4];

static uint32_t firstTime[4];
static uint32_t finished[4];
static uint32_t initialValueTip[4];
static uint32_t initialValueRing[4];

static uint32_t pedal1Factor;
static uint32_t pedal2Factor;
static uint32_t pedal3Factor;
static uint32_t pedal4Factor;

static uint32_t pedal1Min;
static uint32_t pedal2Min;
static uint32_t pedal3Min;
static uint32_t pedal4Min;

static uint32_t pedal1Max;
static uint32_t pedal2Max;
static uint32_t pedal3Max;
static uint32_t pedal4Max;

static uint32_t pedal1;
static uint32_t lastPedal1;
static uint32_t pedal2;
static uint32_t lastPedal2;
static uint32_t pedal3;
static uint32_t lastPedal3;
static uint32_t pedal4;
static uint32_t lastPedal4;

static uint32_t pedal1Behaviour;
static uint32_t pedal2Behaviour;
static uint32_t pedal3Behaviour;
static uint32_t pedal4Behaviour;

static int32_t  lastPitchbend;
static uint32_t pitchbendZero;

static uint32_t pbSignalIsSmall;
static uint32_t pbTestTime;
static uint32_t pbTestMode;
static uint32_t pbRampMode;
static int32_t  pbRamp;
static int32_t  pbRampInc;
static uint32_t benderTable[33] = {};  // contains the chosen aftertouch curve

static uint32_t lastAftertouch;
static uint32_t atTable[33] = {};  // contains the chosen aftertouch curve

//========= ribbons ========

// default calibration tables
static int32_t RIBBON_DEFAULT_CALIBRATION_TABLE_X[] = {
  154, 257, 388, 516, 645, 765, 866, 947, 1044, 1149, 1260, 1376, 1484, 1598, 1710, 1818, 1931, 2034,
  2149, 2251, 2365, 2480, 2595, 2713, 2826, 2956, 3089, 3243, 3384, 3544, 3712, 3872, 4048
};

static int32_t RIBBON_DEFAULT_CALIBRATION_TABLE_Y[] = {
  0, 712, 1198, 1684, 2170, 2655, 3141, 3627, 4113, 4599, 5085, 5571, 6057, 6542, 7028, 7514, 8000,
  8500, 8993, 9494, 9994, 10495, 10995, 11496, 11996, 12497, 12997, 13498, 13998, 14499, 14999, 15500, 16000
};

static LIB_interpol_data_T RIBBON_DEFAULT_CALIBRATION_DATA = {
  sizeof(RIBBON_DEFAULT_CALIBRATION_TABLE_X) / sizeof(RIBBON_DEFAULT_CALIBRATION_TABLE_X[0]),
  RIBBON_DEFAULT_CALIBRATION_TABLE_X,
  RIBBON_DEFAULT_CALIBRATION_TABLE_Y
};

// user calibration tables
static int32_t ribbon_1_calibration_table_x[33];
static int32_t ribbon_1_calibration_table_y[33];
static int32_t ribbon_2_calibration_table_x[33];
static int32_t ribbon_2_calibration_table_y[33];

static LIB_interpol_data_T ribbon_1_calibration_data = {
  sizeof(ribbon_1_calibration_table_x) / sizeof(ribbon_1_calibration_table_x[0]),
  ribbon_1_calibration_table_x,
  ribbon_1_calibration_table_y
};

static LIB_interpol_data_T ribbon_2_calibration_data = {
  sizeof(ribbon_2_calibration_table_x) / sizeof(ribbon_2_calibration_table_x[0]),
  ribbon_2_calibration_table_x,
  ribbon_2_calibration_table_y
};

// type for working variables
typedef struct
{
  int32_t              last;           ///< last raw value (0...4095)
  int32_t              touch;          ///< flag for "touch begins"
  int32_t              behavior;       ///< bit field: Bit 0:return/#non-return, Bit 1:relative/#absolute
  int32_t              relFactor;      ///< scale factor for relative movement
  uint32_t             isEditControl;  ///< flag for usage as "Edit Control Slider"
  uint32_t             editBehavior;   ///< flag absolute/#relative
  uint32_t             incBase;        ///< base value for relative increment
  int32_t              output;         ///< processed output value (0...16000)
  int32_t              threshold;      ///< touch threshold
  LIB_interpol_data_T* calibration;    ///< pointer to calibration data to be used
  uint8_t              ipcId;          ///< ID to fetch raw data from M0 kernel
  uint32_t             paramId;        ///< ID for TCD param set
  uint32_t             hwSourceId;     ///< ID for BB message

} Ribbon_Data_T;

// working variables
static Ribbon_Data_T ribbon[2];  // two ribbons
#define RIB1 0
#define RIB2 1

#if 0
static int32_t lastRibbon1;
static int32_t ribbon1Touch;
static int32_t ribbon1Factor;  // 1024 * 16000 / MaxValue

static int32_t lastRibbon2;
static int32_t ribbon2Touch;
static int32_t ribbon2Factor;  // 1024 * 16000 / MaxValue

static uint32_t ribbon1Behaviour;
static uint32_t ribbon2Behaviour;
static int32_t  ribbonRelFactor;

static uint32_t ribbon1IsEditControl;
static uint32_t ribbon1EditBehaviour;

static uint32_t ribbon1IncBase;
static uint32_t ribbon2IncBase;
static int32_t  ribbon1Output;
static int32_t  ribbon2Output;
#endif

static uint32_t suspend;

/*****************************************************************************
* @brief	ADC_WORK_Generate_BenderTable -
* @param	0: soft, 1: normal, 2: hard
******************************************************************************/
void ADC_WORK_Generate_BenderTable(uint32_t curve)
{
  float_t range = 8000.0;  // separate processing of absolute values for positive and negative range

  float_t s = 0.5;

  switch (curve)  // s defines the curve shape
  {
    case 0:     // soft
      s = 0.0;  // y = x
      break;
    case 1:     // normal
      s = 0.5;  // y = 0.5 * x + 0.5 * x^3
      break;
    case 2:     // hard
      s = 1.0;  // y = x^3
      break;
    default:
      /// Error
      break;
  }

  uint32_t i_max = 32;
  uint32_t i;
  float    x;

  for (i = 0; i <= i_max; i++)
  {
    x = (float) i / (float) i_max;

    benderTable[i] = (uint32_t)(range * x * ((1.0 - s) + s * x * x));
  }
}

/*****************************************************************************
* @brief	ADC_WORK_Generate_AftertouchTable -
* @param	0: soft, 1: normal, 2: hard
******************************************************************************/
void ADC_WORK_Generate_AftertouchTable(uint32_t curve)
{
  float_t range = 16000.0;  // full TCD range

  float_t s = 0.7;

  switch (curve)  // s defines the curve shape
  {
    case 0:     // soft
      s = 0.0;  // y = x
      break;
    case 1:     // normal
      s = 0.7;  // y = 0.3 * x + 0.7 * x^6
      break;
    case 2:      // hard
      s = 0.95;  // y = 0.05 * x + 0.95 * x^6
      break;
    default:
      /// Error
      break;
  }

  uint32_t i_max = 32;
  uint32_t i;
  float    x;

  for (i = 0; i <= i_max; i++)
  {
    x = (float) i / (float) i_max;

    atTable[i] = (uint32_t)(range * x * ((1.0 - s) + s * x * x * x * x * x));
  }
}

/*****************************************************************************
* @brief	ADC_WORK_Init -
******************************************************************************/
void ADC_WORK_Init(void)
{
  uint32_t i;

  for (i = 0; i < 4; i++)
  {
    pedalDetected[i] = 0;
    tipPullup[i]     = 0;
    inverted[i]      = 0;
    tipActive[i]     = 1;

    checkPedal[i] = 0;

    firstTime[i]        = 1;
    finished[i]         = 0;
    initialValueTip[i]  = 0;
    initialValueRing[i] = 0;
  }

  pedal1Factor = 0;
  pedal2Factor = 0;
  pedal3Factor = 0;
  pedal4Factor = 0;

  pedal1Min = 2000;
  pedal2Min = 2000;
  pedal3Min = 2000;
  pedal4Min = 2000;

  pedal1Max = 2200;
  pedal2Max = 2200;
  pedal3Max = 2200;
  pedal4Max = 2200;

  pedal1     = 0;
  lastPedal1 = 0;
  pedal2     = 0;
  lastPedal2 = 0;
  pedal3     = 0;
  lastPedal3 = 0;
  pedal4     = 0;
  lastPedal4 = 0;

  pedal1Behaviour = 0;
  pedal2Behaviour = 0;
  pedal3Behaviour = 0;
  pedal4Behaviour = 0;

  lastPitchbend = 0;
  pitchbendZero = 2048;

  pbSignalIsSmall = 0;
  pbTestTime      = 0;
  pbTestMode      = 0;
  pbRampMode      = 0;
  pbRamp          = 0;
  pbRampInc       = 0;
  ADC_WORK_Generate_BenderTable(1);

  lastAftertouch = 0;
  ADC_WORK_Generate_AftertouchTable(1);

  // initialize ribbon data
  for (i = 0; i <= 1; i++)
  {
    ribbon[i].last          = 0;
    ribbon[i].touch         = 0;
    ribbon[i].behavior      = 0;    // Abs, Non-Return
    ribbon[i].relFactor     = 256;  // == 1.0
    ribbon[i].isEditControl = 0;
    ribbon[i].editBehavior  = 0;
    ribbon[i].incBase       = 0;
    ribbon[i].output        = 0;
    ribbon[i].calibration   = &RIBBON_DEFAULT_CALIBRATION_DATA;             // use default calibration
    ribbon[i].threshold     = 7 * ribbon[i].calibration->x_values[0] / 10;  // set threshold to 70% of lowest raw value
    ribbon[i].ipcId         = (i == 0 ? EMPHASE_IPC_RIBBON_1_ADC : EMPHASE_IPC_RIBBON_2_ADC);
    ribbon[i].paramId       = (i == 0 ? PARAM_ID_RIBBON_1 : PARAM_ID_RIBBON_2);
    ribbon[i].hwSourceId    = (i == 0 ? HW_SOURCE_ID_RIBBON_1 : HW_SOURCE_ID_RIBBON_2);
  }

#if 0
  lastRibbon1   = 0;
  ribbon1Touch  = 0;
  ribbon1Factor = (1024 * 16000) / RIBBON_SPAN;  // 3880;  /// muss (auto-)calibrierbar werden !!!

  lastRibbon2   = 0;
  ribbon2Touch  = 0;
  ribbon2Factor = (1024 * 16000) / RIBBON_SPAN;  // 3880;  /// muss (auto-)calibrierbar werden !!!

  ribbon1Behaviour = 0;    // abs, Non-Return
  ribbon2Behaviour = 0;    // abs, Non-Return
  ribbonRelFactor  = 256;  // factor 1.0

  ribbon1IsEditControl = 0;
  ribbon1EditBehaviour = 0;

  ribbon1IncBase = 0;
  ribbon2IncBase = 0;
  ribbon1Output  = 0;
  ribbon2Output  = 0;
#endif

  suspend = 0;

  Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_1_STATE, PEDAL_DEFAULT_OFF);
  Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_2_STATE, PEDAL_DEFAULT_OFF);
  Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_3_STATE, PEDAL_DEFAULT_OFF);
  Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_4_STATE, PEDAL_DEFAULT_OFF);
}

/******************************************************************************/
/** @param[in]	pedalId: 0..3
*******************************************************************************/
void ADC_WORK_Check_Pedal_Start(uint32_t pedalId)
{
  switch (pedalId)
  {
    case 0:
    {
      Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_1_STATE, PEDAL_CHECK_PINCONFIG);
      break;
    }
    case 1:
    {
      Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_2_STATE, PEDAL_CHECK_PINCONFIG);
      break;
    }
    case 2:
    {
      Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_3_STATE, PEDAL_CHECK_PINCONFIG);
      break;
    }
    case 3:
    {
      Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_4_STATE, PEDAL_CHECK_PINCONFIG);
      break;
    }
    default:
      break;
  }
  firstTime[pedalId]  = 1;
  finished[pedalId]   = 0;
  checkPedal[pedalId] = 1;
}

/******************************************************************************/
/** @param[in]	pedalId: 0..3
*******************************************************************************/
void ADC_WORK_Check_Pedal_Cancel(uint32_t pedalId)
{
  pedalDetected[pedalId] = 0;
  checkPedal[pedalId]    = 0;
}

/******************************************************************************/
/** @param[in]	pedalId: 0..3

	Pedal types:
	SWITCH_OPENING		pull-up on tip		non-inverted	tip active
	SWITCH_CLOSING 		pull-up on tip		inverted		tip active
	POTI_TIP_ACTIVE	 	pull-up on ring		non-inverted	tip active
	POTI_RING_ACTIVE	 	pull-up on tip		non-inverted	ring active
*******************************************************************************/
#if 0
static void CheckPedal(uint32_t pedalId)
{
	uint32_t valueTip;
	uint32_t valueRing;

	switch (pedalId)
	{
	case 0:
		valueTip = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_1_ADC_TIP);
		valueRing = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_1_ADC_RING);
		break;

	case 1:
		valueTip = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_2_ADC_TIP);
		valueRing = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_2_ADC_RING);
		break;

	case 2:
		valueTip = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_3_ADC_TIP);
		valueRing = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_3_ADC_RING);
		break;

	case 3:
		valueTip = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_4_ADC_TIP);
		valueRing = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_4_ADC_RING);
		break;

	default:
		return;
	}

	if (firstTime[pedalId])
	{
		initialValueTip[pedalId] = valueTip;
		initialValueRing[pedalId] = valueRing;

		firstTime[pedalId] = 0;
	}
	else if (finished[pedalId] == 0)		// test is running, waiting for changes by pressing the pedal
	{
		if (initialValueRing[pedalId] < GROUND_THRESHOLD)	// a switch or variable resistor between Tip and Ground (or a poti with an active Ring close to Ground)
		{
			if (valueTip < initialValueTip[pedalId] - CHANGE_FOR_DETECTION)			// increasing current between Tip and Ground
			{
				tipPullup[pedalId] = 1;		// SWITCH_CLOSING
				inverted[pedalId] = 1;
				tipActive[pedalId] = 1;
				finished[pedalId] = 1;
			}
			else if (valueTip > initialValueTip[pedalId] + CHANGE_FOR_DETECTION)	// decreasing current between Tip and Ground
			{
				tipPullup[pedalId] = 1;		// SWITCH_OPENING
				inverted[pedalId] = 0;
				tipActive[pedalId] = 1;
				finished[pedalId] = 1;
			}
		}

		if ( (valueRing <= valueTip)
			&& ( (valueRing > initialValueRing[pedalId] + CHANGE_FOR_DETECTION)
			  || (valueRing < initialValueRing[pedalId] - CHANGE_FOR_DETECTION) ) )
		{
			tipPullup[pedalId] = 1;		// POTI_RING_ACTIVE
			inverted[pedalId] = 0;
			tipActive[pedalId] = 0;
			finished[pedalId] = 1;
		}
		else if ( (valueTip <= valueRing)
				 && ( (valueTip > initialValueTip[pedalId] + CHANGE_FOR_DETECTION)
				   || (valueTip < initialValueTip[pedalId] - CHANGE_FOR_DETECTION) ) )
		{
			tipPullup[pedalId] = 0;		// POTI_TIP_ACTIVE
			inverted[pedalId] = 0;
			tipActive[pedalId] = 1;
			finished[pedalId] = 1;
		}
	}
	else 	// finished[pedalId] == 1
	{
		pedalDetected[pedalId] = 0;		// reset for a fresh pedal calibration
		checkPedal[pedalId] = 0;
		finished[pedalId] = 0;
	}
}
#endif

/*****************************************************************************
* @brief	SendEditMessageToBB - when using a ribbon as an edit control,
* 			the messages are sent by calls to this function
* @param	paramId: parameter Ids (254 ... 289), see: nl_tcd_param_work.h
* @param	value: ribbon position (0 ... 16000)
* @param	inc: position increment (theoretically -16000 ... 16000)
******************************************************************************/
static void SendEditMessageToBB(uint32_t paramId, uint32_t value, int32_t inc)
{
  if (ribbon[RIB1].editBehavior)  // 1: the ribbon sends the absolute value
  {
    BB_MSG_WriteMessage2Arg(BB_MSG_TYPE_EDIT_CONTROL, paramId, value);  // sends the value as an Edit Control message; results being displayed on the upper Ribbon
    BB_MSG_SendTheBuffer();                                             /// später mit somethingToSend = 1; !!!
  }
  else  // 0: the ribbon sends the increment
  {
    uint16_t inc16;

    if (inc < 0)
    {
      if (inc < -8000)  // limiting it to a 14-bit range like with other controls
      {
        inc = -8000;
      }

      inc16 = 0x8000 | (-inc);  // 0x8000 is the sign bit, -inc is the absolute

      BB_MSG_WriteMessage2Arg(BB_MSG_TYPE_EDIT_CONTROL, paramId, inc16);  // sends the increment as an Edit Control message; results being displayed on the upper Ribbon
      BB_MSG_SendTheBuffer();                                             /// später mit somethingToSend = 1; !!!
    }
    else if (inc > 0)
    {
      if (inc > 8000)  // limiting it to a 14-bit range like with other controls
      {
        inc = 8000;
      }

      inc16 = inc;

      BB_MSG_WriteMessage2Arg(BB_MSG_TYPE_EDIT_CONTROL, paramId, inc16);  // sends the increment as an Edit Control message; results being displayed on the upper Ribbon
      BB_MSG_SendTheBuffer();                                             /// später mit somethingToSend = 1; !!!
    }
  }
}

/// "return"-Behaviour braucht eigene Funktion, die mit einem Timer-Task aufgerufen wird
/// mit Abfrage, ob das Ribbon losgelassen wurde. Sprungartiges Zurücksetzen.

/*****************************************************************************
* @brief	SendParamMessageToBB - sending the values of the 8 physical
* 			controls (a.k.a. "Hardware Sources")
* @param	paramId: parameter Ids (254 ... 289), see: nl_tcd_param_work.h
* @param	value: control position (0 ... 16000)
******************************************************************************/
#if 0
static void SendParamMessageToBB(uint32_t paramId, uint32_t value)
{
	BB_MSG_WriteMessage2Arg(BB_MSG_TYPE_PARAMETER, paramId, value);			//  sendet neue Position als Parameter-Message

	BB_MSG_SendTheBuffer();			/// später mit somethingToSend = 1; !!!
}
#endif

void WriteHWValueForBB(uint32_t hwSourceId, uint32_t value)
{
  bbSendValue[hwSourceId] = value | 0x80000;  // the bit is set to check for values to send, will be masked out
}

void ADC_WORK_SendBBMessages(void)  // is called as a regular COOS task
{
  uint32_t i;
  uint32_t send = 0;

  //  bbSendValue[HW_SOURCE_ID_RIBBON_1] = 391 | 0x80000;
  //  bbSendValue[HW_SOURCE_ID_RIBBON_2] = 392 | 0x80000;

  for (i = 0; i < NUM_HW_SOURCES; i++)
  {
    if (bbSendValue[i] > 0)
    {
      if (BB_MSG_WriteMessage2Arg(BB_MSG_TYPE_PARAMETER, hwParamId[i], (bbSendValue[i] & 0xFFFF)) > -1)
      {
        bbSendValue[i] = 0;
        send           = 1;
      }
    }
  }

  if (send == 1)
  {
    BB_MSG_SendTheBuffer();  /// später mit somethingToSend = 1; !!!
  }
}

#if 0
/*****************************************************************************
* @brief	LinearizeRibbon -
******************************************************************************/

static int32_t LinearizeRibbon(int32_t val)
{
  // val -= 310; ???? wat'n dat ????

  if (val < 0)
    val = 0;

  if (val < 8000)
  {
    if (val < 4000)
    {
      val += (val * RIBBON_CORR_4000) >> 10;
    }
    else
    {
      val += ((val - 4000) * RIBBON_CORR_8000 + (8000 - val) * RIBBON_CORR_4000) >> 10;
    }
  }
  else
  {
    if (val < 12000)
    {
      val += ((val - 8000) * RIBBON_CORR_12000 + (12000 - val) * RIBBON_CORR_8000) >> 10;
    }
    else
    {
      val += ((16000 - val) * RIBBON_CORR_12000) >> 10;

      if (val > 16000)
      {
        val = 16000;
      }
    }
  }

  return val;
}
#endif

/*****************************************************************************
* @brief	ADC_WORK_SetRibbon1Behaviour
* @param	0: Abs + Non-Return, 1: Abs + Return, 2: Rel + Non-Return, 3: Rel + Return
******************************************************************************/
void ADC_WORK_SetRibbon1Behaviour(uint32_t behaviour)
{
  ribbon[RIB1].behavior = behaviour;
  if (ribbon[RIB1].isEditControl == 0)  // initialization if working as a HW source
  {
    if ((ribbon[RIB1].behavior == 1) || (ribbon[RIB1].behavior == 3))  // "return" behaviour
    {
      ribbon[RIB1].output = 8000;
      // PARAM_Set(PARAM_ID_RIBBON_1, 8000);
      WriteHWValueForBB(HW_SOURCE_ID_RIBBON_1, 8000);
    }
  }
}

uint32_t ADC_WORK_GetRibbon1Behaviour(void)
{
  if (ribbon[RIB1].behavior & 0x01)
    return RETURN_TO_CENTER;
  else
    return NON_RETURN;
}

/*****************************************************************************
* @brief	ADC_WORK_SetRibbonRelFactor -
* @param
******************************************************************************/
void ADC_WORK_SetRibbonRelFactor(uint32_t factor)
{
  ribbon[RIB1].relFactor = factor;
  ribbon[RIB2].relFactor = factor;
}

/*****************************************************************************
* @brief	ADC_WORK_SetRibbon2Behaviour -
* @param	0: Abs + Non-Return, 1: Abs + Return, 2: Rel + Non-Return, 3: Rel + Return
******************************************************************************/
void ADC_WORK_SetRibbon2Behaviour(uint32_t behaviour)
{
  ribbon[RIB2].behavior = behaviour;
  if (ribbon[RIB2].isEditControl == 0)  // initialization if working as a HW source
  {
    if ((ribbon[RIB2].behavior == 1) || (ribbon[RIB2].behavior == 3))  // "return" behaviour
    {
      ribbon[RIB2].output = 8000;
      // PARAM_Set(PARAM_ID_RIBBON_2, 8000);
      WriteHWValueForBB(HW_SOURCE_ID_RIBBON_2, 8000);
    }
  }
}

uint32_t ADC_WORK_GetRibbon2Behaviour(void)
{
  if (ribbon[RIB2].behavior & 0x01)
    return RETURN_TO_CENTER;
  else
    return NON_RETURN;
}

/*****************************************************************************
* @brief	ADC_WORK_SetRibbon1EditMode -
* @param	0: Play, 1: (Parameter) Edit
******************************************************************************/
void ADC_WORK_SetRibbon1EditMode(uint32_t mode)
{
  ribbon[RIB1].isEditControl = mode;
}

/*****************************************************************************
* @brief	ADC_WORK_SetRibbon1EditBehaviour -
* @param	0: Relative + Non-Return, 1: Absolute + Non-Return
******************************************************************************/
void ADC_WORK_SetRibbon1EditBehaviour(uint32_t behaviour)
{
  ribbon[RIB1].editBehavior = behaviour;
}

/*****************************************************************************
* @brief	ADC_WORK_SetPedal1Behaviour -
* @param	0: Non-Return, 1: Return to Zero, 2: Return to Center
******************************************************************************/
void ADC_WORK_SetPedal1Behaviour(uint32_t behaviour)
{
  pedal1Behaviour = behaviour;

  if (pedal1Behaviour == RETURN_TO_ZERO)
  {
    // PARAM_Set(PARAM_ID_PEDAL_1, 0);
    WriteHWValueForBB(HW_SOURCE_ID_PEDAL_1, 0);
    TEST_Output(0, 0);
  }
  else if (pedal1Behaviour == RETURN_TO_CENTER)
  {
    // PARAM_Set(PARAM_ID_PEDAL_1, 8000);
    WriteHWValueForBB(HW_SOURCE_ID_PEDAL_1, 8000);
    TEST_Output(0, 8000);
  }
}

uint32_t ADC_WORK_GetPedal1Behaviour(void)
{
  return pedal1Behaviour;
}

/*****************************************************************************
* @brief	ADC_WORK_SetPedal2Behaviour -
* @param	0: Non-Return, 1: Return to Zero, 2: Return to Center
******************************************************************************/
void ADC_WORK_SetPedal2Behaviour(uint32_t behaviour)
{
  pedal2Behaviour = behaviour;

  if (pedal2Behaviour == RETURN_TO_ZERO)
  {
    // PARAM_Set(PARAM_ID_PEDAL_2, 0);
    WriteHWValueForBB(HW_SOURCE_ID_PEDAL_2, 0);
    TEST_Output(1, 0);
  }
  else if (pedal2Behaviour == RETURN_TO_CENTER)
  {
    // PARAM_Set(PARAM_ID_PEDAL_2, 8000);
    WriteHWValueForBB(HW_SOURCE_ID_PEDAL_2, 8000);
    TEST_Output(1, 8000);
  }
}

uint32_t ADC_WORK_GetPedal2Behaviour(void)
{
  return pedal2Behaviour;
}

/*****************************************************************************
* @brief	ADC_WORK_SetPedal3Behaviour -
* @param	0: Non-Return, 1: Return to Zero, 2: Return to Center
******************************************************************************/
void ADC_WORK_SetPedal3Behaviour(uint32_t behaviour)
{
  pedal3Behaviour = behaviour;

  if (pedal3Behaviour == RETURN_TO_ZERO)
  {
    // PARAM_Set(PARAM_ID_PEDAL_3, 0);
    WriteHWValueForBB(HW_SOURCE_ID_PEDAL_3, 0);
    TEST_Output(2, 0);
  }
  else if (pedal3Behaviour == RETURN_TO_CENTER)
  {
    // PARAM_Set(PARAM_ID_PEDAL_3, 8000);
    WriteHWValueForBB(HW_SOURCE_ID_PEDAL_3, 8000);
    TEST_Output(2, 8000);
  }
}

uint32_t ADC_WORK_GetPedal3Behaviour(void)
{
  return pedal3Behaviour;
}

/*****************************************************************************
* @brief	ADC_WORK_SetPedal4Behaviour -
* @param	0: Non-Return, 1: Return to Zero, 2: Return to Center
******************************************************************************/
void ADC_WORK_SetPedal4Behaviour(uint32_t behaviour)
{
  pedal4Behaviour = behaviour;

  if (pedal4Behaviour == RETURN_TO_ZERO)
  {
    // PARAM_Set(PARAM_ID_PEDAL_4, 0);
    WriteHWValueForBB(HW_SOURCE_ID_PEDAL_4, 0);
    TEST_Output(3, 0);
  }
  else if (pedal4Behaviour == RETURN_TO_CENTER)
  {
    // PARAM_Set(PARAM_ID_PEDAL_4, 8000);
    WriteHWValueForBB(HW_SOURCE_ID_PEDAL_4, 8000);
    TEST_Output(3, 8000);
  }
}

uint32_t ADC_WORK_GetPedal4Behaviour(void)
{
  return pedal4Behaviour;
}

/*****************************************************************************
* @brief	ADC_WORK_SetRibbonCalibration -
* @param	length: # of words in data array
* @param	data: array containing calibration data sets for the ribbons
******************************************************************************/
// TODO Test this !
void ADC_WORK_SetRibbonCalibration(uint16_t length, uint16_t* data)
{
  if (length != 33 * 4)  // data must contain X and Y sets (33 points) for each ribbon
    return;

  int i;

  for (i = 0; i < 33; i++)
    ribbon_1_calibration_table_x[i] = (int32_t) *data++;
  for (i = 0; i < 33; i++)
    ribbon_1_calibration_table_y[i] = (int32_t) *data++;
  for (i = 0; i < 33; i++)
    ribbon_2_calibration_table_x[i] = (int32_t) *data++;
  for (i = 0; i < 33; i++)
    ribbon_2_calibration_table_y[i] = (int32_t) *data++;

  // Integrity checks
  for (i = 0; i < 33; i++)
  {
    if (ribbon_1_calibration_table_x[i] < 100  // lowest raw value on touch is sure > 100 (140 typ.)
        || ribbon_1_calibration_table_x[i] > 4095
        || ribbon_2_calibration_table_x[i] < 100  // lowest raw value on touch is sure > 100 (140 typ.)
        || ribbon_2_calibration_table_x[i] > 4095
        || ribbon_1_calibration_table_y[i] < 0
        || ribbon_1_calibration_table_y[i] > 16000
        || ribbon_2_calibration_table_y[i] < 0
        || ribbon_2_calibration_table_y[i] > 16000)
      return;  // x- (y-values) are not in valid range for raw values (output values)
  }
  for (i = 1; i < 33; i++)
  {
    if (ribbon_1_calibration_table_x[i] <= ribbon_1_calibration_table_x[i - 1]
        || ribbon_2_calibration_table_x[i] <= ribbon_2_calibration_table_x[i - 1])
      return;  // x-values are not monotonically rising
  }

  ribbon[RIB1].calibration = &ribbon_1_calibration_data;
  ribbon[RIB1].threshold   = 7 * ribbon[RIB1].calibration->x_values[0] / 10;  // set threshold to 70% of lowest raw value
  ribbon[RIB2].calibration = &ribbon_2_calibration_data;
  ribbon[RIB2].threshold   = 7 * ribbon[RIB2].calibration->x_values[0] / 10;  // set threshold to 70% of lowest raw value
}

// TODO test this !
static void ProcessRibbons(void)
{
  int32_t value;
  int32_t valueToSend;

  BB_MSG_WriteMessage2Arg(BB_MSG_TYPE_RIBBON_RAW, Emphase_IPC_PlayBuffer_Read(ribbon[RIB1].ipcId),
		  Emphase_IPC_PlayBuffer_Read(ribbon[RIB2].ipcId));

  for (int i = 0; i <= 1; i++)
  {
    uint32_t send        = 0;
    uint32_t touchBegins = 0;
    int32_t  inc         = 0;

    value = Emphase_IPC_PlayBuffer_Read(ribbon[i].ipcId);

    if (value > ribbon[i].last + 1)  // rising values (min. +2)
    {
      if (value > ribbon[i].threshold)  // above the touch threshold
      {
        if (ribbon[i].touch == 0)
        {
          ribbon[i].touch = 1;
          touchBegins     = 1;
        }
        valueToSend = LIB_InterpolateValue(ribbon[i].calibration, value);
        send        = 1;
      }
      ribbon[i].last = value;
    }
    else if (value + 1 < ribbon[i].last)  // falling values (min. -2)
    {
      if (value > ribbon[i].threshold)  // above the touch threshold; ribbon1Touch is already on, because the last value was higher
      {
        // outputs the previous value (one sample delay) to avoid sending samples of the falling edge of a touch release
        valueToSend = LIB_InterpolateValue(ribbon[i].calibration, ribbon[i].last);
        send        = 1;
      }
      else  // below the touch threshold
      {
        ribbon[i].touch = 0;

        if (ribbon[i].isEditControl == 0)  // working as a play control
        {
          if ((ribbon[i].behavior == 1) || (ribbon[i].behavior == 3))  // "return" behaviour
          {
            PARAM_Set(ribbon[i].paramId, 8000);
            WriteHWValueForBB(ribbon[i].hwSourceId, 8000);

            ribbon[i].output = 8000;
          }
        }
      }
      ribbon[i].last = value;
    }

    if (send)
    {
      if (touchBegins)  // in the incremental mode the jump to the touch position has to be ignored
      {
        inc         = 0;
        touchBegins = 0;
      }
      else
        inc = valueToSend - ribbon[i].incBase;

      ribbon[i].incBase = valueToSend;

      if (ribbon[i].isEditControl)
      {
        inc = (inc * ribbon[i].relFactor) / 256;
        SendEditMessageToBB(ribbon[i].paramId, valueToSend, inc);
      }
      else
      {
        if (ribbon[i].behavior < 2)  // absolute
        {
          ribbon[i].output = valueToSend;

          PARAM_Set(ribbon[i].paramId, ribbon[i].output);
          WriteHWValueForBB(ribbon[i].hwSourceId, ribbon[i].output);
        }
        else  // relative
        {
          if (inc != 0)
          {
            inc = (inc * ribbon[i].relFactor) / 256;
            ribbon[i].output += inc;

            if (ribbon[i].output < 0)
              ribbon[i].output = 0;
            if (ribbon[i].output > 16000)
              ribbon[i].output = 16000;

            PARAM_Set(ribbon[i].paramId, ribbon[i].output);
            WriteHWValueForBB(ribbon[i].hwSourceId, ribbon[i].output);
          }
        }
      }
    }
  }  // for all ribbons
}

/*****************************************************************************
* @brief	ADC_WORK_Process -
******************************************************************************/

void ADC_WORK_Suspend(void)
{
  suspend = 1;
}

void ADC_WORK_Resume(void)
{
  suspend = 0;
}

void ADC_WORK_Process(void)
{
  if (suspend)
  {
    return;
  }

  int32_t value;
  int32_t valueToSend;

  //==================== Pedal 1

  if (Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_1_DETECT))
  {
    if (checkPedal[0] == 1)
    {
      /// CheckPedal(0);
    }
    else if (pedalDetected[0] == 0)  // the pedal has recently been plugged (re-plugging can also be used to reset the auto-calibration)
    {
      if (tipPullup[0] == 1)
      {
        Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_1_STATE, PEDAL_TIP_TO_5V);
      }
      else if (tipPullup[0] == 0)
      {
        Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_1_STATE, PEDAL_RING_TO_5V);
      }

      pedal1Min = 1800;  /// Problem, wenn beim Stecken der Pedal-Detekt vor den ADCs anspricht und ein kurzgeschlossener Kontakt als Min interpretiert wird ???
      pedal1Max = 2200;

      pedalDetected[0] = 1;
    }
    else  // normal mode
    {
      if (tipActive[0])
      {
        value = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_1_ADC_TIP);
      }
      else
      {
        value = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_1_ADC_RING);
      }

      if (value < pedal1Min)
      {
        pedal1Min    = value;
        pedal1Factor = (1024 * 16000) / (pedal1Max - pedal1Min);  // later we will right-shift by 10 bits
      }                                                           /// pedal1Min langsam wieder incrementieren, um auch an steigende Minimalwerte anpassen zu können ?
      else if (value > pedal1Max)
      {
        pedal1Max    = value;
        pedal1Factor = (1024 * 16000) / (pedal1Max - pedal1Min);  // later we will right-shift by 10 bits
      }                                                           /// pedal1Max langsam wieder decrementieren, um auch an fallende Maximalwerte anpassen zu können ?

      if (((value > pedal1)                       // input signal is rising
           && ((pedal1 > lastPedal1)              // AND output was rising before
               || (value > pedal1 + DELTA)))      // OR difference is larger than delta
          || ((value < pedal1)                    // OR input signal is falling
              && ((pedal1 < lastPedal1)           // AND output was falling before
                  || (value + DELTA < pedal1))))  // OR difference is larger than delta
      {
        valueToSend = ((value - pedal1Min) * pedal1Factor) >> 10;

        PARAM_Set(PARAM_ID_PEDAL_1, valueToSend);
        WriteHWValueForBB(HW_SOURCE_ID_PEDAL_1, valueToSend);
        TEST_Output(0, valueToSend);

        lastPedal1 = pedal1;
        pedal1     = value;
      }
    }
  }
  else
  {
    if (pedalDetected[0] == 1)
    {
      Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_1_STATE, PEDAL_DEFAULT_OFF);
      pedalDetected[0] = 0;
    }
  }

  //==================== Pedal 2

  if (Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_2_DETECT))
  {
    if (checkPedal[1] == 1)
    {
      /// CheckPedal(1);
    }
    else if (pedalDetected[1] == 0)  // the pedal has recently been plugged (re-plugging can also be used to reset the auto-calibration)
    {
      if (tipPullup[0] == 1)
      {
        Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_2_STATE, PEDAL_TIP_TO_5V);
      }
      else if (tipPullup[0] == 0)
      {
        Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_2_STATE, PEDAL_RING_TO_5V);
      }

      pedal2Min = 1800;  /// Problem, wenn beim Stecken der Pedal-Detekt vor den ADCs anspricht und ein kurzgeschlossener Kontakt als Min interpretiert wird ???
      pedal2Max = 2200;

      pedalDetected[1] = 1;
    }
    else  // normal mode
    {
      if (tipActive[1])
      {
        value = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_2_ADC_TIP);
      }
      else
      {
        value = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_2_ADC_RING);
      }

      if (value < pedal2Min)
      {
        pedal2Min    = value;
        pedal2Factor = (1024 * 16000) / (pedal2Max - pedal2Min);  // later we will right-shift by 10 bits
      }                                                           /// pedal2Min langsam wieder incrementieren, um auch an steigende Minimalwerte anpassen zu können ?
      else if (value > pedal2Max)
      {
        pedal2Max    = value;
        pedal2Factor = (1024 * 16000) / (pedal2Max - pedal2Min);  // later we will right-shift by 10 bits
      }                                                           /// pedal2Max langsam wieder decrementieren, um auch an fallende Maximalwerte anpassen zu können ?

      if (((value > pedal2)                       // input signal is rising
           && ((pedal2 > lastPedal2)              // AND output was rising before
               || (value > pedal2 + DELTA)))      // OR difference is larger than delta
          || ((value < pedal2)                    // OR input signal is falling
              && ((pedal2 < lastPedal2)           // AND output was falling before
                  || (value + DELTA < pedal2))))  // OR difference is larger than delta
      {
        valueToSend = ((value - pedal2Min) * pedal2Factor) >> 10;

        PARAM_Set(PARAM_ID_PEDAL_2, valueToSend);
        WriteHWValueForBB(HW_SOURCE_ID_PEDAL_2, valueToSend);
        TEST_Output(1, valueToSend);

        lastPedal2 = pedal2;
        pedal2     = value;
      }
    }
  }
  else
  {
    if (pedalDetected[1] == 1)
    {
      Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_2_STATE, PEDAL_DEFAULT_OFF);

      pedalDetected[1] = 0;
    }
  }

  //==================== Pedal 3

  if (Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_3_DETECT))
  {
    if (checkPedal[2] == 1)
    {
      /// CheckPedal(2);
    }
    else if (pedalDetected[2] == 0)  // the pedal has recently been plugged (re-plugging can also be used to reset the auto-calibration)
    {
      if (tipPullup[0] == 1)
      {
        Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_3_STATE, PEDAL_TIP_TO_5V);
      }
      else if (tipPullup[0] == 0)
      {
        Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_3_STATE, PEDAL_RING_TO_5V);
      }

      pedal3Min = 1800;  /// Problem, wenn beim Stecken der Pedal-Detekt vor den ADCs anspricht und ein kurzgeschlossener Kontakt als Min interpretiert wird ???
      pedal3Max = 2200;

      pedalDetected[2] = 1;
    }
    else  // normal mode
    {
      if (tipActive[2])
      {
        value = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_3_ADC_TIP);
      }
      else
      {
        value = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_3_ADC_RING);
      }

      if (value < pedal3Min)
      {
        pedal3Min    = value;
        pedal3Factor = (1024 * 16000) / (pedal3Max - pedal3Min);  // later we will right-shift by 10 bits
      }                                                           /// pedal3Min langsam wieder incrementieren, um auch an steigende Minimalwerte anpassen zu können ?
      else if (value > pedal3Max)
      {
        pedal3Max    = value;
        pedal3Factor = (1024 * 16000) / (pedal3Max - pedal3Min);  // later we will right-shift by 10 bits
      }                                                           /// pedal3Max langsam wieder decrementieren, um auch an fallende Maximalwerte anpassen zu können ?

      if (((value > pedal3)                       // input signal is rising
           && ((pedal3 > lastPedal3)              // AND output was rising before
               || (value > pedal3 + DELTA)))      // OR difference is larger than delta
          || ((value < pedal3)                    // OR input signal is falling
              && ((pedal3 < lastPedal3)           // AND output was falling before
                  || (value + DELTA < pedal3))))  // OR difference is larger than delta
      {
        valueToSend = ((value - pedal3Min) * pedal3Factor) >> 10;

        PARAM_Set(PARAM_ID_PEDAL_3, valueToSend);
        WriteHWValueForBB(HW_SOURCE_ID_PEDAL_3, valueToSend);
        TEST_Output(2, valueToSend);

        lastPedal3 = pedal3;
        pedal3     = value;
      }
    }
  }
  else
  {
    if (pedalDetected[2] == 1)
    {
      Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_3_STATE, PEDAL_DEFAULT_OFF);
      pedalDetected[2] = 0;
    }
  }

  //==================== Pedal 4

  if (Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_4_DETECT))
  {
    if (checkPedal[3] == 1)
    {
      /// CheckPedal(3);
    }
    else if (pedalDetected[3] == 0)  // the pedal has recently been plugged (re-plugging can also be used to reset the auto-calibration)
    {
      if (tipPullup[0] == 1)
      {
        Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_4_STATE, PEDAL_TIP_TO_5V);
      }
      else if (tipPullup[0] == 0)
      {
        Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_4_STATE, PEDAL_RING_TO_5V);
      }

      pedal4Min = 1800;  /// Problem, wenn beim Stecken der Pedal-Detekt vor den ADCs anspricht und ein kurzgeschlossener Kontakt als Min interpretiert wird ???
      pedal4Max = 2200;

      pedalDetected[3] = 1;
    }
    else  // normal mode
    {
      if (tipActive[3])
      {
        value = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_4_ADC_TIP);
      }
      else
      {
        value = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PEDAL_4_ADC_RING);
      }

      if (value < pedal4Min)
      {
        pedal4Min    = value;
        pedal4Factor = (1024 * 16000) / (pedal4Max - pedal4Min);  // later we will right-shift by 10 bits
      }                                                           /// pedal4Min langsam wieder incrementieren, um auch an steigende Minimalwerte anpassen zu können ?
      else if (value > pedal4Max)
      {
        pedal4Max    = value;
        pedal4Factor = (1024 * 16000) / (pedal4Max - pedal4Min);  // later we will right-shift by 10 bits
      }                                                           /// pedal4Max langsam wieder decrementieren, um auch an fallende Maximalwerte anpassen zu können ?

      if (((value > pedal4)                       // input signal is rising
           && ((pedal4 > lastPedal4)              // AND output was rising before
               || (value > pedal4 + DELTA)))      // OR difference is larger than delta
          || ((value < pedal4)                    // OR input signal is falling
              && ((pedal4 < lastPedal4)           // AND output was falling before
                  || (value + DELTA < pedal4))))  // OR difference is larger than delta
      {
        valueToSend = ((value - pedal4Min) * pedal4Factor) >> 10;

        PARAM_Set(PARAM_ID_PEDAL_4, valueToSend);
        WriteHWValueForBB(HW_SOURCE_ID_PEDAL_4, valueToSend);
        TEST_Output(3, valueToSend);

        lastPedal4 = pedal4;
        pedal4     = value;
      }
    }
  }
  else
  {
    if (pedalDetected[3] == 1)
    {
      Emphase_IPC_PlayBuffer_Write(EMPHASE_IPC_PEDAL_4_STATE, PEDAL_DEFAULT_OFF);
      pedalDetected[3] = 0;
    }
  }

  //==================== Pitchbender

  value = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_PITCHBENDER_ADC);  // 0 ... 4095

  value = value - pitchbendZero;  // -2048 ... 2047 (after initialization)

  if ((value > -BENDER_SMALL_THRESH) && (value < BENDER_SMALL_THRESH))  // small signals
  {
    if (!pbSignalIsSmall)  // entering the small-signal range
    {
      pbTestTime = 0;
      pbTestMode = 1;  // start testing
    }

    pbSignalIsSmall = 1;
  }
  else  // large signals
  {
    if (pbSignalIsSmall)  // leaving the small-signal range
    {
      pbTestMode = 0;  // discard testing
    }

    pbSignalIsSmall = 0;
  }

  if (pbTestMode)
  {
    if (pbTestTime < BENDER_TEST_PERIOD)
    {
      pbTestTime++;
    }
    else  // test time finished
    {
      pbTestMode = 0;
      pbRampMode = 1;  // start the ramp to zero

      if (value > 0)
      {
        pbRamp    = value;            // absolute amount of shifting
        pbRampInc = BENDER_RAMP_INC;  // positive increment, will shift the zero line up
      }
      else
      {
        pbRamp    = -value;            // absolute amount of shifting
        pbRampInc = -BENDER_RAMP_INC;  // negative increment, will shift the zero line down
      }
    }
  }

  if (pbRampMode)
  {
    pbRamp -= BENDER_RAMP_INC;  // determines the size of the ramp

    if (pbRamp <= 0)  // ramp has reached zero
    {
      pbRampMode = 0;  // and is stopped
    }
    else  // the steps before the ramp reaches zero
    {
      pitchbendZero += pbRampInc;  // shifting the zero line = negative feedback
    }
  }

  if (value != lastPitchbend)
  {
    if (value > BENDER_DEADRANGE)  // is in the positive work range
    {
      valueToSend = value - BENDER_DEADRANGE;  // absolute amount

      valueToSend = valueToSend * BENDER_FACTOR;  // saturation factor

      if (valueToSend > 2047 * 4096)
      {
        valueToSend = 8000;  // clipping
      }
      else
      {
        valueToSend = valueToSend >> 12;  // 0 ... 2047 (11 Bit)

        uint32_t fract = valueToSend & 0x3F;                                                         // lower 6 bits used for interpolation
        uint32_t index = valueToSend >> 6;                                                           // upper 5 bits (0...31) used as index in the table
        valueToSend    = (benderTable[index] * (64 - fract) + benderTable[index + 1] * fract) >> 6;  // (0...8000) * 64 / 64
      }

      valueToSend = 8000 + valueToSend;  // 8001 ... 16000

      PARAM_Set(PARAM_ID_PITCHBEND, valueToSend);
      WriteHWValueForBB(HW_SOURCE_ID_PITCHBEND, valueToSend);
      TEST_Output(4, valueToSend);
    }
    else if (value < -BENDER_DEADRANGE)  // is in the negative work range
    {
      valueToSend = -BENDER_DEADRANGE - value;  // absolute amount

      valueToSend = valueToSend * BENDER_FACTOR;  // saturation factor

      if (valueToSend > 2047 * 4096)
      {
        valueToSend = 8000;  // clipping
      }
      else
      {
        valueToSend = valueToSend >> 12;  // 0 ... 2047 (11 Bit)

        uint32_t fract = valueToSend & 0x3F;                                                         // lower 6 bits used for interpolation
        uint32_t index = valueToSend >> 6;                                                           // upper 5 bits (0...31) used as index in the table
        valueToSend    = (benderTable[index] * (64 - fract) + benderTable[index + 1] * fract) >> 6;  // (0...8000) * 64 / 64
      }

      valueToSend = 8000 - valueToSend;  // 7999 ... 0

      PARAM_Set(PARAM_ID_PITCHBEND, valueToSend);
      WriteHWValueForBB(HW_SOURCE_ID_PITCHBEND, valueToSend);
      TEST_Output(4, valueToSend);
    }
    else  // is in the dead range
    {
      if ((lastPitchbend > BENDER_DEADRANGE) || (lastPitchbend < -BENDER_DEADRANGE))  // was outside of the dead range before
      {
        PARAM_Set(PARAM_ID_PITCHBEND, 8000);
        WriteHWValueForBB(HW_SOURCE_ID_PITCHBEND, 8000);
        TEST_Output(4, 8000);
      }
    }

    lastPitchbend = value;
  }

  //==================== Aftertouch

  value = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_AFTERTOUCH_ADC);

  if (value != lastAftertouch)
  {
    if (value > AT_DEADRANGE)  // outside of the dead range
    {
      valueToSend = value - AT_DEADRANGE;

      valueToSend = value * AT_FACTOR;  // saturation factor

      if (valueToSend > 4095 * 4096)
      {
        valueToSend = 16000;
      }
      else
      {
        valueToSend = valueToSend >> 12;  // 0 ... 4095

        uint32_t fract = valueToSend & 0x7F;                                                  // lower 7 bits used for interpolation
        uint32_t index = valueToSend >> 7;                                                    // upper 5 bits (0...31) used as index in the table
        valueToSend    = (atTable[index] * (128 - fract) + atTable[index + 1] * fract) >> 7;  // (0...16000) * 128 / 128
      }

      PARAM_Set(PARAM_ID_AFTERTOUCH, valueToSend);
      WriteHWValueForBB(HW_SOURCE_ID_AFTERTOUCH, valueToSend);
      TEST_Output(5, valueToSend);
    }
    else  // inside of the dead range
    {
      if (lastAftertouch > AT_DEADRANGE)  // was outside of the dead range before   /// define
      {
        PARAM_Set(PARAM_ID_AFTERTOUCH, 0);
        WriteHWValueForBB(HW_SOURCE_ID_AFTERTOUCH, 0);
        TEST_Output(5, 0);
      }
    }

    lastAftertouch = value;
  }

  ProcessRibbons();
//==================== Ribbon 1
#if 0
  uint32_t send        = 0;
  uint32_t touchBegins = 0;
  int32_t  inc         = 0;

  value = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_RIBBON_1_ADC);

  if (value > lastRibbon1 + 1)  // rising values (min. +2)
  {
    if (value > RIBBON_THRESHOLD)  // above the touch threshold
    {
      if (ribbon1Touch == 0)
      {
        ribbon1Touch = 1;
        touchBegins  = 1;
      }

      valueToSend = ((value - (RIBBON_MIN + 1)) * ribbon1Factor) / 1024;  // ribbon1Factor is upscaled by 1024
      send        = 1;
    }

    lastRibbon1 = value;
  }
  else if (value + 1 < lastRibbon1)  // falling values (min. -2)
  {
    if (value > RIBBON_THRESHOLD)  // above the touch threshold; ribbon1Touch is already on, because the last value was higher
    {
      valueToSend = ((lastRibbon1 - (RIBBON_MIN + 1)) * ribbon1Factor) / 1024;  // ribbon1Factor is upscaled by 1024
                                                                                // outputs the previous value (one sample delay) to avoid sending samples of the falling edge of a touch release
      send = 1;
    }
    else  // below the touch threshold
    {
      ribbon1Touch = 0;

      if (ribbon1IsEditControl == 0)  // working as a play control
      {
        if ((ribbon1Behaviour == 1) || (ribbon1Behaviour == 3))  // "return" behaviour
        {
          PARAM_Set(PARAM_ID_RIBBON_1, 8000);
          WriteHWValueForBB(HW_SOURCE_ID_RIBBON_1, 8000);
          TEST_Output(6, 8000);

          ribbon1Output = 8000;
        }
      }
    }

    lastRibbon1 = value;
  }

  if (send)
  {
    valueToSend = LinearizeRibbon(valueToSend);

    if (touchBegins)  // in the incremental mode the jump to the touch position has to be ignored
    {
      inc         = 0;
      touchBegins = 0;
    }
    else
    {
      inc = valueToSend - ribbon1IncBase;
    }

    ribbon1IncBase = valueToSend;

    if (ribbon1IsEditControl)
    {
      inc = (inc * ribbonRelFactor) / 256;

      SendEditMessageToBB(PARAM_ID_RIBBON_1, valueToSend, inc);
    }
    else
    {
      if (ribbon1Behaviour < 2)  // absolute
      {
        ribbon1Output = valueToSend;

        PARAM_Set(PARAM_ID_RIBBON_1, ribbon1Output);
        WriteHWValueForBB(HW_SOURCE_ID_RIBBON_1, ribbon1Output);  /// 16000 - ribbon1Output /// war invertierter Modus für Update-Test
        TEST_Output(6, valueToSend);
      }
      else  // relative
      {
        if (inc != 0)
        {
          inc = (inc * ribbonRelFactor) / 256;

          ribbon1Output += inc;

          if (ribbon1Output < 0)
          {
            ribbon1Output = 0;
          }
          else if (ribbon1Output > 16000)
          {
            ribbon1Output = 16000;
          }

          PARAM_Set(PARAM_ID_RIBBON_1, ribbon1Output);
          WriteHWValueForBB(HW_SOURCE_ID_RIBBON_1, ribbon1Output);
          TEST_Output(6, valueToSend);
        }
      }
    }

    send = 0;
  }

  /// schneller Abfall unter die Threshold wird als Touch-off interpretiert
  /// bei Abwärtsbewegungen ein, zwei Samples abwarten, d.h. leichte Latenz
  /// Hold-Verhalten zur Überbrückung kurzer Aussetzer durch geringen Druck des Fingers ???

  //==================== Ribbon 2

  value = Emphase_IPC_PlayBuffer_Read(EMPHASE_IPC_RIBBON_2_ADC);

  if (value > lastRibbon2 + 1)  // rising values (min. +2)
  {
    if (value > RIBBON_THRESHOLD)  // above the touch threshold
    {
      if (ribbon2Touch == 0)
      {
        ribbon2Touch = 1;
        touchBegins  = 1;
      }

      valueToSend = ((value - (RIBBON_MIN + 1)) * ribbon2Factor) / 1024;  // ribbon2Factor is upscaled by 1024
      send        = 1;
    }

    lastRibbon2 = value;
  }
  else if (value + 1 < lastRibbon2)  // falling values (min. -2)
  {
    if (value > RIBBON_THRESHOLD)  // above the touch threshold; ribbon2Touch is already on, because the last value was higher
    {
      valueToSend = ((lastRibbon2 - (RIBBON_MIN + 1)) * ribbon2Factor) / 1024;  // ribbon2Factor is upscaled by 1024
                                                                                // outputs the previous value (one sample delay) to avoid sending samples of the falling edge of a touch release
      send = 1;
    }
    else  // below the touch threshold
    {
      ribbon2Touch = 0;

      if ((ribbon2Behaviour == 1) || (ribbon2Behaviour == 3))  // "return" behaviour
      {
        PARAM_Set(PARAM_ID_RIBBON_2, 8000);
        WriteHWValueForBB(HW_SOURCE_ID_RIBBON_2, 8000);
        TEST_Output(7, 8000);

        ribbon2Output = 8000;
      }
    }

    lastRibbon2 = value;
  }

  if (send)
  {
    valueToSend = LinearizeRibbon(valueToSend);

    if (touchBegins)  // in the incremental mode the jump to the touch position has to be ignored
    {
      inc = 0;
    }
    else
    {
      inc = valueToSend - ribbon2IncBase;
    }

    ribbon2IncBase = valueToSend;

    if (ribbon2Behaviour < 2)  // absolute
    {
      ribbon2Output = valueToSend;

      PARAM_Set(PARAM_ID_RIBBON_2, ribbon2Output);
      WriteHWValueForBB(HW_SOURCE_ID_RIBBON_2, ribbon2Output);
      TEST_Output(7, valueToSend);
    }
    else  // relative
    {
      if (inc != 0)
      {
        inc = (inc * ribbonRelFactor) / 256;

        ribbon2Output += inc;

        if (ribbon2Output < 0)
        {
          ribbon2Output = 0;
        }
        else if (ribbon2Output > 16000)
        {
          ribbon2Output = 16000;
        }

        PARAM_Set(PARAM_ID_RIBBON_2, ribbon2Output);
        WriteHWValueForBB(HW_SOURCE_ID_RIBBON_2, ribbon2Output);
        TEST_Output(7, valueToSend);
      }
    }
  }
#endif
}
