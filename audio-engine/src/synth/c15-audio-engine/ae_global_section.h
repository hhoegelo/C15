#pragma once

/******************************************************************************/
/** @file       ae_global_section.h
    @date
    @version    1.7-0
    @author     M. Seeber
    @brief      new container for all global parameters and dsp
    @todo
*******************************************************************************/

#include "smoother_handle.h"
#include "ae_info.h"
#include "nltoolbox.h"

// master (volume, tune), scale, test tone (freq, amp, follow_key), soft clip
class GlobalSection
{
 public:
  GlobalSignals m_signals;
  float m_out_l = 0.0f, m_out_r = 0.0f, m_note_shift = 0.0f;
  GlobalSection();
  void init(const float _sampleInc);
  void add_copy_audio_id(const uint32_t _smootherId, const uint32_t _signalId);
  void add_copy_fast_id(const uint32_t _smootherId, const uint32_t _signalId);
  void add_copy_slow_id(const uint32_t _smootherId, const uint32_t _signalId);
  void start_sync(const uint32_t _id, const float _dest);
  void start_audio(const uint32_t _id, const float _dx, const float _dest);
  void start_fast(const uint32_t _id, const float _dx, const float _dest);
  void start_slow(const uint32_t _id, const float _dx, const float _dest);
  float key_position(const uint32_t _pos);
  void render_audio(const float _left0, const float _right0, const float _left1, const float _right1);
  void render_fast();
  void render_slow();
  void update_tone_amplitude(const float _db);
  void update_tone_frequency(const float _freq);
  void update_tone_mode(const uint32_t _mode);

 private:
  SmootherHandle<C15::Smoothers::Global_Sync, C15::Smoothers::Global_Audio, C15::Smoothers::Global_Fast,
                 C15::Smoothers::Global_Slow>
      m_smoothers;
  float m_signal[3] = {};
  float m_sampleInc = 0.0f, m_tonePhase = 0.0f, m_toneAmp = 0.0f, m_toneFreq = 0.0f;
  uint32_t m_combinationMode = 0;
  void postProcess_audio();
  void postProcess_fast();
  void postProcess_slow();
  float evalScale(const uint32_t _pos);
};
