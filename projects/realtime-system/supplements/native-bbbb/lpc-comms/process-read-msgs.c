#include "process-read-msgs.h"

//===========================

#define BB_MSG_TYPE_PARAMETER    0x0400  // direction: output; arguments(uint16): 2, 1x parameter ID , 1x data
#define BB_MSG_TYPE_EDIT_CONTROL 0x0500  // not used
#define BB_MSG_TYPE_SETTING      0x0700  // direction: input;  arguments (uint16): 2, 1x SETTING_ID_*, 1x data
#define BB_MSG_TYPE_NOTIFICATION 0x0800  // direction: output; arguments(uint16): 2, 1x type, 1x value
#define BB_MSG_TYPE_ASSERTION    0x0900  // direction: output; arguments(uint16): n (string)
#define BB_MSG_TYPE_REQUEST      0x0A00  // direction: input;  argument (uint16): 1, 1x REQUEST_ID_*
#define BB_MSG_TYPE_HEARTBEAT    0x0B00  // direction: output; arguments(uint16): 4, 4x uint16 (==uint64)
#define BB_MSG_TYPE_MUTESTATUS   0x0C00  // direction: output; argument (uint16): 1, 1x bit pattern
#define BB_MSG_TYPE_RIBBON_CAL   0x0D00  // direction: input; arguments(uint16): 134, 134x data [2x (33x 34x)]
#define BB_MSG_TYPE_SENSORS_RAW  0x0E00  // direction: output; arguments(uint16): 13, sensor raw data (see nl_tcd_adc_work.c)
#define BB_MSG_TYPE_EHC_CONFIG   0x0F00  // direction: input;  arguments (uint16): 2, 1x command, 1x data
#define BB_MSG_TYPE_EHC_DATA     0x1000  // direction: output;  arguments(uint16): ??, (see nl_ehc_ctrl.c)
#define BB_MSG_TYPE_COOS_DATA    0x1200  // direction: output;  arguments (uint16): 4

//----- Setting Ids:

#define SETTING_ID_PLAY_MODE_UPPER_RIBBON_BEHAVIOUR 0   // ==> BIT 0 set if (returnMode == RETURN), ...
#define SETTING_ID_PLAY_MODE_LOWER_RIBBON_BEHAVIOUR 1   // ... BIT 1 set if (touchBehaviour == RELATIVE)
#define SETTING_ID_BASE_UNIT_UI_MODE                3   // ==> PLAY = 0, PARAMETER_EDIT = 1
#define SETTING_ID_EDIT_MODE_RIBBON_BEHAVIOUR       4   // ==> RELATIVE = 0, ABSOLUTE = 1
#define SETTING_ID_UPPER_RIBBON_REL_FACTOR          9   // ==> tTcdRange(256, 2560)
#define SETTING_ID_LOWER_RIBBON_REL_FACTOR          10  // ==> tTcdRange(256, 2560)
#define SETTING_ID_VELOCITY_CURVE                   11  // ==> VERY_SOFT = 0, SOFT = 1, NORMAL = 2, HARD = 3, VERY_HARD = 4

// SETTING_ID_PEDAL_x_TYPE must be a monotonic rising sequence
#define SETTING_ID_PEDAL_1_TYPE 26  // ==> PotTipActive  = 0
#define SETTING_ID_PEDAL_2_TYPE 27  // ... PotRingActive = 1
#define SETTING_ID_PEDAL_3_TYPE 28  // ... SwitchClosing = 2 // aka momentary switch, normally open
#define SETTING_ID_PEDAL_4_TYPE 29  // ... SwitchOpening = 3 // aka momentary switch, normally closed

#define SETTING_ID_AFTERTOUCH_CURVE 30  // SOFT = 0, NORMAL = 1, HARD = 2
#define SETTING_ID_BENDER_CURVE     31  // SOFT = 0, NORMAL = 1, HARD = 2

// new setting ID's
#define SETTING_ID_SOFTWARE_MUTE_OVERRIDE 0xFF01  // Software Mute Override
#define SETTING_ID_SEND_RAW_SENSOR_DATA   0xFF02  // direction: input; arguments(uint16): 1, flag (!= 0)
#define SETTING_ID_SEND_FORCED_KEY        0xFF03  // direction: input; arguments(uint16): 1, midi key number
#define SETTING_ID_ENABLE_EHC             0xFF04  // direction: input; arguments(uint16): 1, flag (!= 0)

//----- Request Ids:
#define REQUEST_ID_SW_VERSION     0x0000
#define REQUEST_ID_UNMUTE_STATUS  0x0001
#define REQUEST_ID_EHC_DATA       0x0002
#define REQUEST_ID_CLEAR_EEPROM   0x0003
#define REQUEST_ID_COOS_DATA      0x0004
#define REQUEST_ID_EHC_EEPROMSAVE 0x0005

//----- Notification Ids:
#define NOTIFICATION_ID_SW_VERSION     0x0000
#define NOTIFICATION_ID_UNMUTE_STATUS  0x0001
#define NOTIFICATION_ID_EHC_DATA       0x0002
#define NOTIFICATION_ID_CLEAR_EEPROM   0x0003
#define NOTIFICATION_ID_COOS_DATA      0x0004
#define NOTIFICATION_ID_EHC_EEPROMSAVE 0x0005

// ==================================================================================
#define HW_SOURCE_ID_PEDAL_1    0
#define HW_SOURCE_ID_PEDAL_2    1
#define HW_SOURCE_ID_PEDAL_3    2
#define HW_SOURCE_ID_PEDAL_4    3
#define HW_SOURCE_ID_PITCHBEND  4
#define HW_SOURCE_ID_AFTERTOUCH 5
#define HW_SOURCE_ID_RIBBON_1   6
#define HW_SOURCE_ID_RIBBON_2   7
#define HW_SOURCE_ID_PEDAL_5    8
#define HW_SOURCE_ID_PEDAL_6    9
#define HW_SOURCE_ID_PEDAL_7    10
#define HW_SOURCE_ID_PEDAL_8    11
#define HW_SOURCE_ID_LAST_KEY   12
#define LAST_PARAM_ID           12

// ==================================================================================
#define SUP_UNMUTE_STATUS_IS_VALID           (0b1000000000000000)  // status has actually been set
#define SUP_UNMUTE_STATUS_JUMPER_OVERRIDE    (0b0000000010000000)  // hardware jumper overriding everything ...
#define SUP_UNMUTE_STATUS_JUMPER_VALUE       (0b0000000001000000)  // ... with this value (1:unmuted)
#define SUP_UNMUTE_STATUS_SOFTWARE_OVERRIDE  (0b0000000000100000)  // software command overriding midi-derived status ...
#define SUP_UNMUTE_STATUS_SOFTWARE_VALUE     (0b0000000000010000)  // ... with this value (1:unmuted)
#define SUP_UNMUTE_STATUS_MIDI_DERIVED       (0b0000000000001000)  // midi-derived status ...
#define SUP_UNMUTE_STATUS_MIDI_DERIVED_VALUE (0b0000000000000100)  // ... with this value (1:unmuted)
#define SUP_UNMUTE_STATUS_HARDWARE_IS_VALID  (0b0000000000000010)  // hardware status is valid (signal from SUP uC was detected) ...
#define SUP_UNMUTE_STATUS_HARDWARE_VALUE     (0b0000000000000001)  // ... with this value (1:unmuted)

// ==================================================================================
char paramNameTable[][LAST_PARAM_ID] = {
  "EHC 1     ",
  "EHC 2     ",
  "EHC 3     ",
  "EHC 4     ",
  "BENDER    ",
  "AFTERTOUCH",
  "RIBBON 1  ",
  "RIBBON 2  ",
  "EHC 5     ",
  "EHC 6     ",
  "EHC 7     ",
  "EHC 8     ",
  "LASTKEY",
};
char paramShortNameTable[][16] = {
  "E1",
  "E2",
  "E3",
  "E4",
  "PB",
  "AT",
  "R1",
  "R2",
  "E5",
  "E6",
  "E7",
  "E8",
  "--",
  "--",
  "--",
  "--",
};

// ==================================================================================
typedef struct
{
  unsigned ctrlId : 3;            // controller number 0...7, aka input (main) ADC channel 0...7, 0/1=J1T/R, 2/3=J2T/R, etc,
  unsigned hwId : 4;              // hardware ID used for messages to AE and PG
  unsigned silent : 1;            // disable messaging to AudioEngine
  unsigned is3wire : 1;           // controller connection type, 0=2wire(rheo/sw/cv), 1=3wire(pot)
  unsigned pullup : 1;            // controller input sensing, 0=unloaded(pot/CV), 1=with pullup(rheo/sw)
  unsigned continuous : 1;        // controller output type, 0=continuous(all), 1=bi-stable(all)
  unsigned polarityInvert : 1;    // invert, or don't, the final output(all)
  unsigned autoHoldStrength : 3;  // controller auto-hold 0..7, 0(off)...4=autoHold-Strength for pot/rheo
  unsigned doAutoRanging : 1;     // enable auto-ranging, or assume static (but adjustable) thresholds/levels
} EHC_ControllerConfig_T;

uint16_t configToUint16(const EHC_ControllerConfig_T c)
{
  uint16_t ret = 0;
  ret |= c.autoHoldStrength << 0;
  ret |= c.continuous << 3;
  ret |= c.doAutoRanging << 4;
  ret |= c.polarityInvert << 5;
  ret |= c.pullup << 6;
  ret |= c.is3wire << 7;
  ret |= c.ctrlId << 8;
  ret |= c.silent << 11;
  ret |= c.hwId << 12;
  return ret;
}
EHC_ControllerConfig_T uint16ToConfig(const uint16_t c)
{
  EHC_ControllerConfig_T ret;
  ret.autoHoldStrength = (c & 0b0000000000000111) >> 0;
  ret.continuous       = (c & 0b0000000000001000) >> 3;
  ret.doAutoRanging    = (c & 0b0000000000010000) >> 4;
  ret.polarityInvert   = (c & 0b0000000000100000) >> 5;
  ret.pullup           = (c & 0b0000000001000000) >> 6;
  ret.is3wire          = (c & 0b0000000010000000) >> 7;
  ret.ctrlId           = (c & 0b0000011100000000) >> 8;
  ret.silent           = (c & 0b0000100000000000) >> 11;
  ret.hwId             = (c & 0b1111000000000000) >> 12;
  return ret;
}

typedef struct
{
  unsigned initialized : 1;    // controller is initialized (has valid setup)
  unsigned pluggedIn : 1;      // controller is plugged in
  unsigned isReset : 1;        // controller is freshly reset
  unsigned outputIsValid : 1;  // controller final output value has been set
  unsigned isAutoRanged : 1;   // controller has finished auto-ranging (always=1 when disabled)
  unsigned isSettled : 1;      // controller output is within 'stable' bounds and step-freezing (not valid for bi-stable)
  unsigned isRamping : 1;      // controller currently does a ramp to the actual value (pot/rheo) (not valid for bi-stable)
  unsigned isSaved : 1;        // controller state has been saved to EEPROM
  unsigned isRestored : 1;     // controller state has been restored from EEPROM

} EHC_ControllerStatus_T;

uint16_t statusToUint16(const EHC_ControllerStatus_T s)
{
  uint16_t ret = 0;
  ret |= s.initialized << 0;
  ret |= s.pluggedIn << 1;
  ret |= s.isReset << 2;
  ret |= s.outputIsValid << 3;
  ret |= s.isAutoRanged << 4;
  ret |= s.isSettled << 5;
  ret |= s.isRamping << 6;
  ret |= s.isSaved << 7;
  ret |= s.isRestored << 8;
  return ret;
}
EHC_ControllerStatus_T uint16ToStatus(const uint16_t s)
{
  EHC_ControllerStatus_T ret;
  ret.initialized   = (s & 0b0000000000000001) >> 0;
  ret.pluggedIn     = (s & 0b0000000000000010) >> 1;
  ret.isReset       = (s & 0b0000000000000100) >> 2;
  ret.outputIsValid = (s & 0b0000000000001000) >> 3;
  ret.isAutoRanged  = (s & 0b0000000000010000) >> 4;
  ret.isSettled     = (s & 0b0000000000100000) >> 5;
  ret.isRamping     = (s & 0b0000000001000000) >> 6;
  ret.isSaved       = (s & 0b0000000010000000) >> 7;
  ret.isRestored    = (s & 0b0000000100000000) >> 8;
  return ret;
}

// ==================================================================================
typedef enum
{
  RED,
  GREEN,
  DEFAULT
} color_t;

void setColor(color_t color)
{
  switch (color)
  {
    case RED:
      printf("\033[0;31m");
      break;
    case GREEN:
      printf("\033[0;32m");
      break;
    case DEFAULT:
      printf("\033[0m");
      break;
  }
}

int greenNotRed(int condition)
{
  setColor(condition ? GREEN : RED);
  return condition;
}

// ==================================================================================
void dump(uint16_t const cmd, uint16_t const len, uint16_t *const data, uint16_t const flags)
{
  if (flags & NO_HEXDUMP)
    return;
  printf("cmd:%04X len:%04X: data:", cmd, len);
  for (int i = 0; i < len; i++)
    printf("%04X ", data[i]);
  printf("\n");
}

// ==================================================================================, uint16_t const flags
void processReadMsgs(uint16_t const cmd, uint16_t const len, uint16_t *const data, uint16_t flags)
{
  int       i;
  uint16_t *p;
  double    last[8], min[8], max[8], scale[8];

  EHC_ControllerConfig_T config[8];
  EHC_ControllerStatus_T status[8];

  switch (cmd)
  {
    case BB_MSG_TYPE_PARAMETER:
      if (flags & NO_PARAMS)
        return;
      dump(cmd, len, data, flags);
      if (len != 2)
      {
        printf("PARAM : wrong length of %d\n", len);
        return;
      }
      if (data[0] > LAST_PARAM_ID)
        return;
      printf("PARAM (HWSID %02d) %s = %d\n", data[0], paramNameTable[data[0]], data[1]);
      return;

    case BB_MSG_TYPE_NOTIFICATION:
      if (flags & NO_NOTIFICATION)
        return;
      dump(cmd, len, data, flags);
      if (len != 2)
      {
        printf("NOTIFICATION : wrong length of %d\n", len);
        return;
      }
      switch (data[0])
      {
        case NOTIFICATION_ID_CLEAR_EEPROM:
          printf("NOTIFICATION : EEPROM cleared\n");
          return;
        case NOTIFICATION_ID_COOS_DATA:
          printf("NOTIFICATION : Task Data sent\n");
          return;
        case NOTIFICATION_ID_SW_VERSION:
          printf("NOTIFICATION : Software Version: %hu\n", data[1]);
          return;
        case NOTIFICATION_ID_EHC_DATA:
          printf("NOTIFICATION : EHC data sent: %s\n", data[1] ? "success" : "failed");
          return;
        case NOTIFICATION_ID_EHC_EEPROMSAVE:
          printf("NOTIFICATION : EHC data save to EEPROM: %s\n", data[1] ? "started successfully" : "postponed/failed");
          return;
        case NOTIFICATION_ID_UNMUTE_STATUS:
          printf("NOTIFICATION : ");
          data[0] = data[1];
          goto ShowMuteStatus;
      }

    case BB_MSG_TYPE_COOS_DATA:
      if (flags & NO_COOSDATA)
        return;
      dump(cmd, len, data, flags);
      if (len != 4)
      {
        printf("TASKS : wrong length of %d\n", len);
        return;
      }
      printf("TASK Scheduler tops: %d overs, %d per slice, task:%dus, scheduler:%dus\n",
             data[0], data[1], 5 * (int) data[2] / 2, 5 * (int) data[3] / 2);
      return;

    case BB_MSG_TYPE_HEARTBEAT:
      if (flags & NO_HEARTBEAT)
        return;
      dump(cmd, len, data, flags);
      if (len != 4)
      {
        printf("HEARTBEAT : wrong length of %d\n", len);
        return;
      }
      uint64_t heartbeat = 0;
      for (int i = 3; i >= 0; i--)
      {
        heartbeat <<= 16;
        heartbeat |= data[i];
      }
      printf("HEARTBEAT : %8llu\n", heartbeat);
      return;

    case BB_MSG_TYPE_SENSORS_RAW:
      if (flags & NO_SENSORSRAW)
        return;
      dump(cmd, len, data, flags);
      if (len != 13)
      {
        printf("HEARTBEAT : wrong length of %d\n", len);
        return;
      }
      printf("RAW SENSORS: ");
      for (int i = 3; i >= 0; i--)
        printf("%c ", (data[0] & (1 << i)) ? '1' : '0');
      printf(" ");
      for (int i = 1; i < 13; i++)
      {
        printf("%04hu ", data[i]);
        if (i == 8 || i == 10)
          printf("/ ");
      }
      printf("\n");
      return;

    case BB_MSG_TYPE_MUTESTATUS:
      if (flags & NO_MUTESTATUS)
        return;
      dump(cmd, len, data, flags);
      if (len != 1)
      {
        printf("MUTESTATUS : wrong length of %d\n", len);
        return;
      }
    ShowMuteStatus:
      printf("MUTESTATUS: valid:%d", data[0] & SUP_UNMUTE_STATUS_IS_VALID ? 1 : 0);
      printf(" jumper:");
      if (data[0] & SUP_UNMUTE_STATUS_JUMPER_OVERRIDE)
        printf("%d", data[0] & SUP_UNMUTE_STATUS_JUMPER_VALUE ? 1 : 0);
      else
        printf("-");
      printf(" software:");
      if (data[0] & SUP_UNMUTE_STATUS_SOFTWARE_OVERRIDE)
        printf("%d", data[0] & SUP_UNMUTE_STATUS_SOFTWARE_VALUE ? 1 : 0);
      else
        printf("-");
      printf(" midi:");
      if (data[0] & SUP_UNMUTE_STATUS_MIDI_DERIVED)
        printf("%d", data[0] & SUP_UNMUTE_STATUS_MIDI_DERIVED_VALUE ? 1 : 0);
      else
        printf("-");
      printf(" physical:");
      if (data[0] & SUP_UNMUTE_STATUS_HARDWARE_IS_VALID)
        printf("%d", data[0] & SUP_UNMUTE_STATUS_HARDWARE_VALUE ? 1 : 0);
      else
        printf("-");
      printf("\n");
      return;

    case BB_MSG_TYPE_EHC_DATA:
      if (flags & NO_EHCDATA)
        return;
      dump(cmd, len, data, flags);
      if (len != 8 * 6)
      {
        printf("EHC DATA : wrong length of %d\n", len);
        return;
      }
      p = (uint16_t *) data;
      for (i = 0; i < 8; i++)
      {
        config[i] = uint16ToConfig(*p++);
        status[i] = uint16ToStatus(*p++);
        last[i]   = *p++;
        min[i]    = *p++;
        max[i]    = *p++;
        scale[i]  = *p++;
      }
      printf("EHC data\nController    ");
      for (i = 0; i < 8; i++)
      {
        greenNotRed(status[i].initialized);
        printf("P%d%c(%d)%c", i / 2 + 1, i & 1 ? 'R' : 'T', i, i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      // config
      printf("CONFIG        ");
      for (i = 0; i < 8; i++)
      {
        greenNotRed(status[i].initialized);
        printf("%04Xh %c", configToUint16(config[i]), i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      printf("  HWSID/SIL   ");
      for (i = 0; i < 8; i++)
      {
        greenNotRed(status[i].initialized);
        printf("%s/%c  %c", paramShortNameTable[config[i].hwId], config[i].silent ? 'S' : '-', i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      printf("  Pot/pUl/Con ");
      for (i = 0; i < 8; i++)
      {
        greenNotRed(status[i].initialized);
        printf("%c/%c/%c %c", config[i].is3wire ? 'P' : '-', config[i].pullup ? 'U' : '-', config[i].continuous ? 'C' : '-', i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      printf("  Inv/aRe/AHS ");
      for (i = 0; i < 8; i++)
      {
        greenNotRed(status[i].initialized);
        printf("%c/%c/%d %c", config[i].polarityInvert ? 'I' : '-', config[i].doAutoRanging ? 'R' : '-', config[i].autoHoldStrength, i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      // status
      printf("STATUS        ");
      for (i = 0; i < 8; i++)
      {
        greenNotRed(status[i].initialized);
        printf("%04Xh %c", statusToUint16(status[i]), i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      printf("  Ini/Plg/rEs ");
      for (i = 0; i < 8; i++)
      {
        greenNotRed(status[i].initialized);
        printf("%c/%c/%c %c", status[i].initialized ? 'I' : '-', status[i].pluggedIn ? 'P' : '-', status[i].isReset ? 'E' : '-', i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      printf("  Rng/Val/Stl ");
      for (i = 0; i < 8; i++)
      {
        greenNotRed(status[i].initialized);
        printf("%c/%c/%c %c", status[i].isAutoRanged ? 'R' : '-', status[i].outputIsValid ? 'V' : '-', status[i].isSettled ? 'S' : '-', i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      printf("  rAm/eeS/eeR ");
      for (i = 0; i < 8; i++)
      {
        greenNotRed(status[i].initialized);
        printf("%c/%c/%c %c", status[i].isRamping ? 'A' : '-', status[i].isSaved ? 'S' : '-', status[i].isRestored ? 'R' : '-', i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      // last
      printf("LAST          ");
      for (i = 0; i < 8; i++)
      {
        if (greenNotRed(config[i].hwId != 15 && status[i].initialized && status[i].pluggedIn && status[i].outputIsValid))
          printf("%5.0lf %c", last[i], i == 7 ? '\n' : ' ');
        else
          printf("      %c", i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      // last %
      printf("LAST %%        ");
      for (i = 0; i < 8; i++)
      {
        if (greenNotRed(config[i].hwId != 15 && status[i].initialized && status[i].pluggedIn && status[i].outputIsValid))
          printf("%5.1lf%%%c", 100 * last[i] / 16000, i == 7 ? '\n' : ' ');
        else
          printf("      %c", i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      // min
      printf("MIN           ");
      for (i = 0; i < 8; i++)
      {
        if (greenNotRed(config[i].hwId != 15 && status[i].initialized))
        {
          if (!(status[i].isAutoRanged))
            setColor(DEFAULT);
          printf("%5.0lf %c", min[i], i == 7 ? '\n' : ' ');
        }
        else
          printf("      %c", i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      // max
      printf("MAX           ");
      for (i = 0; i < 8; i++)
      {
        if (greenNotRed(config[i].hwId != 15 && status[i].initialized))
        {
          if (!(status[i].isAutoRanged))
            setColor(DEFAULT);
          printf("%5.0lf %c", max[i], i == 7 ? '\n' : ' ');
        }
        else
          printf("      %c", i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      // min %
      printf("MIN %%         ");
      for (i = 0; i < 8; i++)
      {
        if (greenNotRed(config[i].hwId != 15 && status[i].initialized))
        {
          if (!(status[i].isAutoRanged))
            setColor(DEFAULT);
          printf("%5.1lf%%%c", 100 * min[i] / scale[i], i == 7 ? '\n' : ' ');
        }
        else
          printf("      %c", i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      // max %
      printf("MAX %%         ");
      for (i = 0; i < 8; i++)
      {
        if (greenNotRed(config[i].hwId != 15 && status[i].initialized))
        {
          if (!(status[i].isAutoRanged))
            setColor(DEFAULT);
          printf("%5.1lf%%%c", 100 * max[i] / scale[i], i == 7 ? '\n' : ' ');
        }
        else
          printf("      %c", i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      // scale
      printf("SCALE         ");
      for (i = 0; i < 8; i++)
      {
        if (greenNotRed(config[i].hwId != 15 && status[i].initialized))
        {
          printf("%5.0lf %c", scale[i], i == 7 ? '\n' : ' ');
        }
        else
          printf("      %c", i == 7 ? '\n' : ' ');
      }
      setColor(DEFAULT);

      /*
      for (i = 0; i < 8; i++)
      {
        config = uint16ToConfig(*p++);
        status = uint16ToStatus(*p++);
        setColor(config.hwId == 15 ? RED : GREEN);
        printf("P%d%c(%d) ", i/2+1, i&1 ? 'R' : 'T', i);
        printf("config (0x%04X) = HWSID:%02d SIL:%d CTRLID:%d POT:%d PUL:%d INV:%d ARAE:%d CONT:%d AHS:%d\n",
               configToUint16(config), config.hwId, config.silent, config.ctrlId, config.is3wire, config.pullup, config.polarityInvert, config.doAutoRanging, config.continuous, config.autoHoldStrength);
        setColor(status.initialized ? GREEN : RED);
        printf("       status (0x%04X) = INI:%c PLUGD:%d RES:%d VALID:%d RANGD:%d SETLD:%d RAMP:%d EES:%d EER:%d\n",
               statusToUint16(status), status.initialized ? '1' : '-', status.pluggedIn, status.isReset,
               status.outputIsValid, status.isAutoRanged, status.isSettled, status.isRamping, status.isSaved, status.isRestored);
        last = *p++;
        if (!status.outputIsValid)
          last = -0.01;
        min   = *p++;
        max   = *p++;
        scale = *p++;
        printf("       last / min / max / scale :  %d(%.1lf%%) / %d(%.1lf%%) / %d(%.1lf%%) / %5d\n",
               (uint16_t) last, 100 * last / 16000, (uint16_t) min, 100 * min / scale, (uint16_t) max, 100 * max / scale, (uint16_t) scale);
        setColor(DEFAULT);
      }
      printf("\n");
*/
      return;
    default:
      if (flags & NO_UNKNOWN)
        return;
      printf("UNKNOWN ");
      flags &= ~NO_HEXDUMP;
      dump(cmd, len, data, flags);
      return;
  }
}
