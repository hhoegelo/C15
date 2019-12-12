#pragma once

#include <stdio.h>
#include <stdint.h>
#include "interpol.h"

namespace Calibrator
{

  class CalToRef
  {
   public:
    enum Options
    {
      Quiet,
      VerboseMessages
    };
    CalToRef(const Options option);
    ~CalToRef();

    void startAddIn(void) const;

    // add in the revVal->dutVal mapping, if possible
    // return 0:success, 1:dropped values, 2:discarded values, 3:fatal error
    int addInSamplePair(uint16_t const binNumber, uint16_t const values);

    void endAddIn(void) const;

    unsigned getValidSamples(void) const;  // returns number of valid sample pairs

    // return : true=success
    bool ProcessData(void);

    // output resulting calibration file
    // param: fp: opened(!) output file, and it won't close it as well!
    // if fp==NULL, stdout is used
    bool OutputData(FILE *fp) const;

   private:
    bool m_verbose;
    unsigned m_totalSamples;
    unsigned m_validSamples;
    unsigned m_droppedSamples;
    unsigned m_discardedSamples;

    static constexpr auto m_MAX_DELTA = 300;  // maximum allowed difference for ref and dut values for a point
    static constexpr auto m_THRESHOLD = 100;  // minimum allowed value (worst-case ribbon threshold)
    static constexpr auto m_RANGE = 4096;     // (ADC) value range
    static constexpr auto m_MAX_DIST = 30;    // max allowed distance between adjacent points
    static constexpr int m_AVG_SIZE = 3;      // must be odd and >= 3 !! TODO find best value!
    static constexpr int m_AVG_CENTER = (m_AVG_SIZE - 1) / 2;
    static constexpr auto m_MONOTONICITY_DELTA = 5;

    struct RawData
    {
      float value;
      int32_t count;
    };

    RawData m_data[m_RANGE];
    uint32_t m_lastBinNumber;
    uint16_t m_lastValue;

    float m_mappedX[m_RANGE];
    float m_mappedY[m_RANGE];
    int m_mappedCount;
    float m_mappedAvgdX[m_RANGE];
    float m_mappedAvgdY[m_RANGE];

    float m_RIBBON_REFERENCE_CALIBRATION_TABLE_Y[33]
        = { 0,     712,   1198,  1684,  2170,  2655,  3141,  3627,  4113,  4599,  5085,
            5571,  6057,  6542,  7028,  7514,  8000,  8500,  8993,  9494,  9994,  10495,
            10995, 11496, 11996, 12497, 12997, 13498, 13998, 14499, 14999, 15500, 16000 };

    float m_RIBBON_REFERENCE_CALIBRATION_TABLE_X[34]
        = { 142,  165,  263,  390,  518,  649,  765,  866,  950,  1047, 1152, 1263, 1376, 1484, 1596, 1710, 1820,
            1930, 2034, 2149, 2251, 2365, 2480, 2595, 2713, 2826, 2954, 3093, 3236, 3390, 3544, 3711, 3872, 4035 };
    float m_ribbon_dut_calibration_data_y[34];

    Interpol::InterpolData m_interpolData;
  };
}

// EOF
