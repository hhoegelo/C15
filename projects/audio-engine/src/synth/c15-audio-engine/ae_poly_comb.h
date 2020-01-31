#pragma once

/******************************************************************************/
/** @file       ae_poly_comb.h
    @date
    @version    1.7-3
    @author     A. Schmied, M. Seeber
    @brief      comb filter
    @todo
*******************************************************************************/

#include <vector>
#include "nltoolbox.h"
#include "ae_info.h"

#define COMB_BUFFER_SIZE 8192

namespace Engine
{
  class PolyCombFilter
  {
   public:
    PolyValue m_out;
    PolyCombFilter();
    void init(const float _samplerate, const uint32_t _upsampleFactor);
    void apply(PolySignals &_signals, const PolyValue &_sampleA, const PolyValue &_sampleB);
    void set(PolySignals &_signals, const float _samplerate, const uint32_t _voiceId);
    void setDelaySmoother(const uint32_t _voiceId);
    void resetDSP();
    float calcDecayGain(const float _decay, const float _frequency);

   private:
    std::vector<PolyValue> m_buffer;
    PolyValue m_hpCoeff_b0, m_hpCoeff_b1, m_hpCoeff_a1, m_hpInStateVar, m_hpOutStateVar;
    PolyValue m_lpCoeff, m_lpStateVar;
    PolyValue m_apCoeff_1, m_apCoeff_2, m_apStateVar_1, m_apStateVar_2, m_apStateVar_3, m_apStateVar_4;
    PolyValue m_decayStateVar, m_delaySamples, m_delayStateVar;
    float m_sampleRate, m_sampleInterval, m_warpConst_PI, m_warpConst_2PI, m_freqClip_2, m_freqClip_4, m_freqClip_24576;
    float m_delayFreqClip, m_delayConst;
    int32_t m_buffer_indx, m_buffer_sz_m1;
  };
}  // namespace Engine
