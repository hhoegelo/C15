#include "dsp_host_dual.h"

using namespace std::chrono_literals;

/******************************************************************************/
/** @file       dsp_host_dual.cpp
    @date
    @version    1.7-3
    @author     M. Seeber
    @brief      new main engine container.
    @todo
*******************************************************************************/

dsp_host_dual::dsp_host_dual()
{
  m_mainOut_L = m_mainOut_R = 0.0f;
  m_layer_mode = C15::Properties::LayerMode::Single;
}

void dsp_host_dual::init(const uint32_t _samplerate, const uint32_t _polyphony)
{
  // BEWARE: we still only support TWO integer multiples of 48000 Hz ("SD": 1 x 48k, "HD": 2 x 48k)
  const uint32_t upsampleFactor = (_samplerate > 96000 ? 96000 : _samplerate) / 48000,
                 upsampleIndex = upsampleFactor - 1;
  if(_samplerate != C15::Config::clock_rates[upsampleIndex][0])
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "invalid sample rate(", _samplerate,
                          "): only 48000 or 96000 Hz allowed!");
  }
  const float samplerate = static_cast<float>(C15::Config::clock_rates[upsampleIndex][0]);
  // init of crucial components: voiceAlloc, conversion, clock, time, fadepoint
  m_alloc.init();
  m_alloc.setSplitPoint(30);  // temporary..?
  m_convert.init();
  m_clock.init(upsampleIndex);
  m_time.init(upsampleIndex);
  m_fade.init(samplerate);
  // proper time init
  m_edit_time.init(C15::Properties::SmootherScale::Linear, 200.0f, 0.0f, 0.1f);
  m_edit_time.m_scaled = scale(m_edit_time.m_scaling, m_edit_time.m_position);
  updateTime(&m_edit_time.m_dx, m_edit_time.m_scaled);
  m_transition_time.init(C15::Properties::SmootherScale::Expon_Env_Time, 1.0f, -20.0f, 0.0f);
  m_transition_time.m_scaled = scale(m_transition_time.m_scaling, m_transition_time.m_position);
  updateTime(&m_transition_time.m_dx, m_transition_time.m_scaled);
  // note: time and reference setting params are currently hard-coded but could also be part of parameter list
  m_reference.init(C15::Properties::SmootherScale::Linear, 80.0f, 400.0f, 0.5f);
  m_reference.m_scaled = scale(m_reference.m_scaling, m_reference.m_position);
  // dsp sections init
  m_global.init(m_time.m_sample_inc);
  m_global.update_tone_amplitude(-6.0f);
  m_global.update_tone_frequency(m_reference.m_scaled);
  m_global.update_tone_mode(0);

  // voice fade stuff I (currently explicit)
  m_poly[0].m_fadeStart = 0;
  m_poly[0].m_fadeEnd = C15::Config::key_count - 1;
  m_poly[0].m_fadeIncrement = 1;
  // init poly dsp: exponentiator, feedback pointers
  m_poly[0].init(&m_global.m_signals, &m_convert, &m_time, &m_z_layers[0], &m_reference.m_scaled, m_time.m_millisecond,
                 env_init_gateRelease, samplerate, upsampleFactor);
  // voice fade stuff II (currently explicit)
  m_poly[1].m_fadeStart = C15::Config::key_count - 1;
  m_poly[1].m_fadeEnd = 0;
  m_poly[1].m_fadeIncrement = -1;
  // init poly dsp: exponentiator, feedback pointers
  m_poly[1].init(&m_global.m_signals, &m_convert, &m_time, &m_z_layers[1], &m_reference.m_scaled, m_time.m_millisecond,
                 env_init_gateRelease, samplerate, upsampleFactor);
  // init mono dsp
  m_mono[0].init(&m_convert, &m_z_layers[0], m_time.m_millisecond, samplerate, upsampleFactor);
  m_mono[1].init(&m_convert, &m_z_layers[1], m_time.m_millisecond, samplerate, upsampleFactor);
  // init parameters by parameter list
  m_params.init_modMatrix();
  for(uint32_t i = 0; i < C15::Config::tcd_elements; i++)
  {
    auto element = C15::ParameterList[i];
    switch(element.m_param.m_type)
    {
      // (global) unmodulateable parameters need their properties and can directly feed to global signals
      case C15::Descriptors::ParameterType::Global_Modulateable:
        m_params.init_global_modulateable(element);
        switch(element.m_ae.m_signal.m_signal)
        {
          case C15::Descriptors::ParameterSignal::Global_Signal:
            switch(element.m_ae.m_smoother.m_clock)
            {
              case C15::Descriptors::SmootherClock::Sync:
                m_global.add_copy_sync_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Audio:
                m_global.add_copy_audio_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Fast:
                m_global.add_copy_fast_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Slow:
                m_global.add_copy_slow_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
            }
            break;
        }
        break;
        // (global) unmodulateable parameters need their properties and can directly feed to global signals
      case C15::Descriptors::ParameterType::Global_Unmodulateable:
        m_params.init_global_unmodulateable(element);
        switch(element.m_ae.m_signal.m_signal)
        {
          case C15::Descriptors::ParameterSignal::Global_Signal:
            switch(element.m_ae.m_smoother.m_clock)
            {
              case C15::Descriptors::SmootherClock::Sync:
                m_global.add_copy_sync_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Audio:
                m_global.add_copy_audio_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Fast:
                m_global.add_copy_fast_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Slow:
                m_global.add_copy_slow_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
            }
            break;
        }
        break;
      // macro controls need to know their position in mod matrix (which hw amounts are relevant)
      case C15::Descriptors::ParameterType::Macro_Control:
        m_params.init_macro(element);
        break;
      // (local) modulateable parameters need their properties and can directly feed to either quasipoly or mono signals
      case C15::Descriptors::ParameterType::Local_Modulateable:
        m_params.init_local_modulateable(element);
        switch(element.m_ae.m_signal.m_signal)
        {
          case C15::Descriptors::ParameterSignal::Quasipoly_Signal:
            switch(element.m_ae.m_smoother.m_clock)
            {
              case C15::Descriptors::SmootherClock::Sync:
                m_poly[0].add_copy_sync_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_poly[1].add_copy_sync_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Audio:
                m_poly[0].add_copy_audio_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_poly[1].add_copy_audio_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Fast:
                m_poly[0].add_copy_fast_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_poly[1].add_copy_fast_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Slow:
                m_poly[0].add_copy_slow_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_poly[1].add_copy_slow_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
            }
            break;
          case C15::Descriptors::ParameterSignal::Mono_Signal:
            switch(element.m_ae.m_smoother.m_clock)
            {
              case C15::Descriptors::SmootherClock::Sync:
                m_mono[0].add_copy_sync_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_mono[1].add_copy_sync_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Audio:
                m_mono[0].add_copy_audio_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_mono[1].add_copy_audio_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Fast:
                m_mono[0].add_copy_fast_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_mono[1].add_copy_fast_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Slow:
                m_mono[0].add_copy_slow_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_mono[1].add_copy_slow_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
            }
            break;
        }
        break;
      // (local) unmodulateable parameters need their properties and can directly feed to either quasipoly or mono signals
      case C15::Descriptors::ParameterType::Local_Unmodulateable:
        m_params.init_local_unmodulateable(element);
        switch(element.m_ae.m_signal.m_signal)
        {
          case C15::Descriptors::ParameterSignal::Quasipoly_Signal:
            switch(element.m_ae.m_smoother.m_clock)
            {
              case C15::Descriptors::SmootherClock::Sync:
                m_poly[0].add_copy_sync_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_poly[1].add_copy_sync_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Audio:
                m_poly[0].add_copy_audio_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_poly[1].add_copy_audio_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Fast:
                m_poly[0].add_copy_fast_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_poly[1].add_copy_fast_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Slow:
                m_poly[0].add_copy_slow_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_poly[1].add_copy_slow_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
            }
            break;
          case C15::Descriptors::ParameterSignal::Mono_Signal:
            switch(element.m_ae.m_smoother.m_clock)
            {
              case C15::Descriptors::SmootherClock::Sync:
                m_mono[0].add_copy_sync_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_mono[1].add_copy_sync_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Audio:
                m_mono[0].add_copy_audio_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_mono[1].add_copy_audio_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Fast:
                m_mono[0].add_copy_fast_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_mono[1].add_copy_fast_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
              case C15::Descriptors::SmootherClock::Slow:
                m_mono[0].add_copy_slow_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                m_mono[1].add_copy_slow_id(element.m_ae.m_smoother.m_index, element.m_ae.m_signal.m_index);
                break;
            }
            break;
        }
        break;
      case C15::Descriptors::ParameterType::Macro_Time:
        auto mc = m_params.init_macro_time(element);
        mc->m_time.m_scaled = scale(mc->m_time.m_scaling, mc->m_time.m_position);
        updateTime(&mc->m_time.m_dx, mc->m_time.m_scaled);
        break;
    }
  }
  // temporary: load initials in order to have valid osc reset params
  //onSettingInitialSinglePreset();

#if __POTENTIAL_IMPROVEMENT_NUMERIC_TESTS__
  PotentialImprovements_RunNumericTests();
#endif

  if(LOG_INIT)
  {
    //nltools::Log::info("dsp_host_dual::init - engine dsp status: global");
    //nltools::Log::info("missing: nltools::msg - reference, initial:", m_reference.m_scaled);
  }
}

C15::ParameterDescriptor dsp_host_dual::getParameter(const int _id)
{
  if((_id > -1) && (_id < C15::Config::tcd_elements))
  {
    if(LOG_DISPATCH)
    {
      if(C15::ParameterList[_id].m_param.m_type != C15::Descriptors::ParameterType::None)
      {
        nltools::Log::info("dispatch(", _id, "):", C15::ParameterList[_id].m_pg.m_group_label_short, "-",
                           C15::ParameterList[_id].m_pg.m_param_label_short);
      }
      else
      {
        nltools::Log::warning(__PRETTY_FUNCTION__, "dispatch(", _id, "): None!");
      }
    }
    return C15::ParameterList[_id];
  }
  if(LOG_FAIL)
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "dispatch(", _id, "):", "failed!");
  }
  return m_invalid_param;
}

void dsp_host_dual::logStatus()
{
  if(LOG_ENGINE_STATUS)
  {
    nltools::Log::info("engine status:");
    nltools::Log::info("-clock(index:", m_clock.m_index, ", fast:", m_clock.m_fast, ", slow:", m_clock.m_slow, ")");
    nltools::Log::info("-output(left:", m_mainOut_L, ", right:", m_mainOut_R, ", mute:", m_fade.getValue(), ")");
    nltools::Log::info("-dsp(dx:", m_time.m_sample_inc, ", ms:", m_time.m_millisecond, ")");
  }
  else if(LOG_ENGINE_EDITS)
  {
    nltools::Log::info("Engine Param Status - - - - - - - - - - - - - - - \n\n");
    for(uint32_t i = 0; i < LOG_PARAMS_LENGTH; i++)
    {
      auto element = C15::ParameterList[LOG_PARAMS[i]];
      switch(element.m_param.m_type)
      {
        case C15::Descriptors::ParameterType::None:
          break;
        case C15::Descriptors::ParameterType::Hardware_Source:
          nltools::Log::info("HW[", LOG_PARAMS[i], "]", element.m_pg.m_group_label_short, ">",
                             element.m_pg.m_param_label_short,
                             ":(pos:", m_params.get_hw_src(element.m_param.m_index)->m_position,
                             ", beh:", static_cast<int>(m_params.get_hw_src(element.m_param.m_index)->m_behavior), ")");
          break;
        case C15::Descriptors::ParameterType::Hardware_Amount:
          nltools::Log::info("HA[", LOG_PARAMS[i], "]", element.m_pg.m_group_label_short, ">",
                             element.m_pg.m_param_label_short,
                             ":(pos:", m_params.get_hw_amt(element.m_param.m_index)->m_position, ")");
          break;
        case C15::Descriptors::ParameterType::Macro_Control:
          nltools::Log::info("MC[", LOG_PARAMS[i], "]", element.m_pg.m_group_label_short, ">",
                             element.m_pg.m_param_label_short,
                             ":(pos:", m_params.get_macro(element.m_param.m_index)->m_position, ")");
          break;
        case C15::Descriptors::ParameterType::Macro_Time:
          nltools::Log::info("MT[", LOG_PARAMS[i], "]", element.m_pg.m_group_label_short, ">",
                             element.m_pg.m_param_label_short,
                             ":(pos:", m_params.get_macro_time(element.m_param.m_index)->m_position,
                             ", scl:", m_params.get_macro_time(element.m_param.m_index)->m_scaled, ")");
          break;
        case C15::Descriptors::ParameterType::Global_Modulateable:
          nltools::Log::info("GT[", LOG_PARAMS[i], "]", element.m_pg.m_group_label_short, ">",
                             element.m_pg.m_param_label_short, ":(pos:",
                             m_params.get_global_target(element.m_param.m_index)
                                 ->polarize(m_params.get_global_target(element.m_param.m_index)->m_position),
                             ", scl:", m_params.get_global_target(element.m_param.m_index)->m_scaled,
                             ", amt:", m_params.get_global_target(element.m_param.m_index)->m_amount,
                             ", src:", static_cast<int>(m_params.get_global_target(element.m_param.m_index)->m_source),
                             ")");
          break;
        case C15::Descriptors::ParameterType::Global_Unmodulateable:
          nltools::Log::info("GD[", LOG_PARAMS[i], "]", element.m_pg.m_group_label_short, ">",
                             element.m_pg.m_param_label_short,
                             ":(pos:", m_params.get_global_direct(element.m_param.m_index)->m_position,
                             ", scl:", m_params.get_global_direct(element.m_param.m_index)->m_scaled, ")");
          break;
        case C15::Descriptors::ParameterType::Local_Modulateable:
          nltools::Log::info("LT[I, ", LOG_PARAMS[i], "]", element.m_pg.m_group_label_short, ">",
                             element.m_pg.m_param_label_short, ":(pos:",
                             m_params.get_local_target(0, element.m_param.m_index)
                                 ->polarize(m_params.get_local_target(0, element.m_param.m_index)->m_position),
                             ", scl:", m_params.get_local_target(0, element.m_param.m_index)->m_scaled,
                             ", amt:", m_params.get_local_target(0, element.m_param.m_index)->m_amount, ", src:",
                             static_cast<int>(m_params.get_local_target(0, element.m_param.m_index)->m_source), ")");
          if(m_layer_mode != C15::Properties::LayerMode::Single)
          {
            nltools::Log::info("LT[II, ", LOG_PARAMS[i], "]", element.m_pg.m_group_label_short, ">",
                               element.m_pg.m_param_label_short, ":(pos:",
                               m_params.get_local_target(1, element.m_param.m_index)
                                   ->polarize(m_params.get_local_target(1, element.m_param.m_index)->m_position),
                               ", scl:", m_params.get_local_target(1, element.m_param.m_index)->m_scaled,
                               ", amt:", m_params.get_local_target(1, element.m_param.m_index)->m_amount, ", src:",
                               static_cast<int>(m_params.get_local_target(1, element.m_param.m_index)->m_source), ")");
          }
          break;
        case C15::Descriptors::ParameterType::Local_Unmodulateable:
          nltools::Log::info("LD[I, ", LOG_PARAMS[i], "]", element.m_pg.m_group_label_short, ">",
                             element.m_pg.m_param_label_short,
                             ":(pos:", m_params.get_local_direct(0, element.m_param.m_index)->m_position,
                             ", scl:", m_params.get_local_direct(0, element.m_param.m_index)->m_scaled, ")");
          if(m_layer_mode != C15::Properties::LayerMode::Single)
          {
            nltools::Log::info("LD[II, ", LOG_PARAMS[i], "]", element.m_pg.m_group_label_short, ">",
                               element.m_pg.m_param_label_short,
                               ":(pos:", m_params.get_local_direct(1, element.m_param.m_index)->m_position,
                               ", scl:", m_params.get_local_direct(1, element.m_param.m_index)->m_scaled, ")");
          }
          break;
      }
    }
    nltools::Log::info("\n\n - - - - - - - - - - - - - - - Engine Param Status");
  }
}

void dsp_host_dual::onMidiMessage(const uint32_t _status, const uint32_t _data0, const uint32_t _data1)
{

  if(LOG_MIDI)
  {
    nltools::Log::info("midiMsg(status:", _status, ", data0:", _data0, ", data1:", _data1, ")");
  }
  const uint32_t ch = _status & 15, st = (_status & 127) >> 4;
  uint32_t arg = 0;
  // LPC MIDI Protocol 1.7 transmits every LPC Message as MIDI PitchBend Message! (avoiding TCD Protocol collisions)
  if(st == 6)
  {
    switch(ch)
    {
      case 0:
        // Pedal 1
        arg = _data1 + (_data0 << 7);
        if(LOG_MIDI_DETAIL)
        {
          nltools::Log::info("midiMsg(source:Pedal1, raw:", arg, ")");
        }
        updateHW(0, static_cast<float>(arg));
        break;
      case 1:
        // Pedal 2
        arg = _data1 + (_data0 << 7);
        if(LOG_MIDI_DETAIL)
        {
          nltools::Log::info("midiMsg(source:Pedal2, raw:", arg, ")");
        }
        updateHW(1, static_cast<float>(arg));
        break;
      case 2:
        // Pedal 3
        arg = _data1 + (_data0 << 7);
        if(LOG_MIDI_DETAIL)
        {
          nltools::Log::info("midiMsg(source:Pedal3, raw:", arg, ")");
        }
        updateHW(2, static_cast<float>(arg));
        break;
      case 3:
        // Pedal 4
        arg = _data1 + (_data0 << 7);
        if(LOG_MIDI_DETAIL)
        {
          nltools::Log::info("midiMsg(source:Pedal4, raw:", arg, ")");
        }
        updateHW(3, static_cast<float>(arg));
        break;
      case 4:
        // Bender
        arg = _data1 + (_data0 << 7);
        if(LOG_MIDI_DETAIL)
        {
          nltools::Log::info("midiMsg(source:Bender, raw:", arg, ")");
        }
        updateHW(4, static_cast<float>(arg));
        break;
      case 5:
        // Aftertouch
        arg = _data1 + (_data0 << 7);
        if(LOG_MIDI_DETAIL)
        {
          nltools::Log::info("midiMsg(source:Aftertouch, raw:", arg, ")");
        }
        updateHW(5, static_cast<float>(arg));
        break;
      case 6:
        // Ribbon 1
        arg = _data1 + (_data0 << 7);
        if(LOG_MIDI_DETAIL)
        {
          nltools::Log::info("midiMsg(source:Ribbon1, raw:", arg, ")");
        }
        updateHW(6, static_cast<float>(arg));
        break;
      case 7:
        // Ribbon 2
        arg = _data1 + (_data0 << 7);
        if(LOG_MIDI_DETAIL)
        {
          nltools::Log::info("midiMsg(source:Ribbon2, raw:", arg, ")");
        }
        updateHW(7, static_cast<float>(arg));
        break;
      case 13:
        // Key Pos (down or up)
        arg = _data1;
        m_key_pos = arg - C15::Config::key_from;
        // key position is only valid within range [36 ... 96]
        m_key_valid = (arg >= C15::Config::key_from) && (arg <= C15::Config::key_to);
        if(LOG_MIDI_DETAIL)
        {
          nltools::Log::info("midiMsg(source:KeyPos, raw:", arg, ", valid:", m_key_valid, ")");
        }
        break;
      case 14:
        // Key Down
        arg = _data1 + (_data0 << 7);
        if(m_key_valid)
        {
          keyDown(static_cast<float>(arg) * m_norm_vel);
        }
        else if(LOG_FAIL)
        {
          nltools::Log::warning(__PRETTY_FUNCTION__, "key_down(pos:", m_key_pos, ") failed!");
        }
        // ...
        break;
      case 15:
        // Key Up
        arg = _data1 + (_data0 << 7);
        if(m_key_valid)
        {
          keyUp(static_cast<float>(arg) * m_norm_vel);
        }
        else if(LOG_FAIL)
        {
          nltools::Log::warning(__PRETTY_FUNCTION__, "key_up(pos:", m_key_pos, ") failed!");
        }
        break;
      default:
        break;
    }
  }
}

void dsp_host_dual::onRawMidiMessage(const uint32_t _status, const uint32_t _data0, const uint32_t _data1)
{
  const uint32_t type = (_status & 127) >> 4;
  uint32_t arg = 0;
  switch(type)
  {
    case 0:
      // note off
      arg = static_cast<uint32_t>(static_cast<float>(_data1) * m_format_vel);
      onMidiMessage(0xED, 0, _data0);            // keyPos
      onMidiMessage(0xEF, arg >> 7, arg & 127);  // keyUp
      break;
    case 1:
      // note on
      arg = static_cast<uint32_t>(static_cast<float>(_data1) * m_format_vel);
      onMidiMessage(0xED, 0, _data0);  // keyPos
      if(arg != 0)
      {
        onMidiMessage(0xEE, arg >> 7, arg & 127);  // keyDown
      }
      else
      {
        onMidiMessage(0xEF, 0, 0);  // keyUp
      }
      break;
    case 3:
      // ctrl chg (hw sources: pedals, ribbons) - hard coded ReMOTE template "MSE 2"
      arg = static_cast<uint32_t>(static_cast<float>(_data1) * m_format_hw);
      switch(_data0)
      {
        case 61:
          // Pedal 1
          onMidiMessage(0xE0, arg >> 7, arg & 127);
          break;
        case 62:
          // Pedal 2
          onMidiMessage(0xE1, arg >> 7, arg & 127);
          break;
        case 63:
          // Pedal 3
          onMidiMessage(0xE2, arg >> 7, arg & 127);
          break;
        case 64:
          // Pedal 4
          onMidiMessage(0xE3, arg >> 7, arg & 127);
          break;
        case 67:
          // Ribbon 1
          onMidiMessage(0xE6, arg >> 7, arg & 127);
          break;
        case 68:
          // Ribbon 2
          onMidiMessage(0xE7, arg >> 7, arg & 127);
          break;
        default:
          break;
      }
      break;
    case 5:
      // mono at (hw source)
      arg = static_cast<uint32_t>(static_cast<float>(_data0) * m_format_hw);
      onMidiMessage(0xE5, arg >> 7, arg & 127);  // hw_at
      break;
    case 6:
      // bend (hw source)
      arg = static_cast<uint32_t>(static_cast<float>(_data0 + (_data1 << 7)) * m_format_pb);
      onMidiMessage(0xE4, arg >> 7, arg & 127);  // hw_bend
      break;
    default:
      break;
  }
}

void dsp_host_dual::onPresetMessage(const nltools::msg::SinglePresetMessage& _msg)
{
  if(LOG_RECALL)
  {
    // log preset with primitive timestamp (for debugging fade events)
    nltools::Log::info("Received Single Preset Message! (@", m_clock.m_index, ")");
  }

  if(m_glitch_suppression)
  {
    // glitch suppression: start outputMute fade
    m_fade.muteAndDo([&] {
      // global flush
      m_poly[0].flushDSP();
      m_poly[1].flushDSP();
      m_mono[0].flushDSP();
      m_mono[1].flushDSP();
      recallSingle(_msg);
    });

    if(LOG_RECALL)
    {
      nltools::Log::info(__PRETTY_FUNCTION__, m_fade.getNumMuteSamplesRendered(), "mute samples rendered.");
    }
  }
  else
  {
    recallSingle(_msg);
  }
}

void dsp_host_dual::onPresetMessage(const nltools::msg::SplitPresetMessage& _msg)
{
  if(LOG_RECALL)
  {
    nltools::Log::info("Received Split Preset Message!, (@", m_clock.m_index, ")");
  }

  if(m_glitch_suppression)
  {
    // glitch suppression: start outputMute fade
    m_fade.muteAndDo([&] {
      // global flush
      m_poly[0].flushDSP();
      m_poly[1].flushDSP();
      m_mono[0].flushDSP();
      m_mono[1].flushDSP();
      recallSplit(_msg);
    });

    if(LOG_RECALL)
    {
      nltools::Log::info(__PRETTY_FUNCTION__, m_fade.getNumMuteSamplesRendered(), "mute samples rendered.");
    }
  }
  else
  {
    recallSplit(_msg);
  }
}

void dsp_host_dual::onPresetMessage(const nltools::msg::LayerPresetMessage& _msg)
{
  if(LOG_RECALL)
  {
    nltools::Log::info("Received Layer Preset Message!, (@", m_clock.m_index, ")");
  }
  if(m_glitch_suppression)
  {
    // glitch suppression: start outputMute fade
    m_fade.muteAndDo([&] {
      // global flush
      m_poly[0].flushDSP();
      m_poly[1].flushDSP();
      m_mono[0].flushDSP();
      m_mono[1].flushDSP();
      recallLayer(_msg);
    });

    if(LOG_RECALL)
    {
      nltools::Log::info(__PRETTY_FUNCTION__, m_fade.getNumMuteSamplesRendered(), "mute samples rendered.");
    }
  }
  else
  {
    recallLayer(_msg);
  }
}

void dsp_host_dual::globalParChg(const uint32_t _id, const nltools::msg::HWSourceChangedMessage& _msg)
{
  auto param = m_params.get_hw_src(_id);
  float inc = -param->m_position;
  if(param->update_behavior(getBehavior(_msg.returnMode)))
  {
    param->update_position(static_cast<float>(_msg.controlPosition));
    if(LOG_EDITS)
    {
      nltools::Log::info("hw_edit(pos:", param->m_position, ", behavior:", static_cast<int>(param->m_behavior), ")");
    }
  }
  else if(param->update_position(static_cast<float>(_msg.controlPosition)))
  {
    if(LOG_EDITS)
    {
      nltools::Log::info("hw_edit(pos:", param->m_position, ")");
    }
    inc += param->m_position;
    hwModChain(param, _id, inc);
  }
}

void dsp_host_dual::globalParChg(const uint32_t _id, const nltools::msg::HWAmountChangedMessage& _msg)
{
  auto param = m_params.get_hw_amt(_id);
  if(param->update_position(static_cast<float>(_msg.controlPosition)))
  {
    if(LOG_EDITS)
    {
      nltools::Log::info("ha_edit(srcId:", param->m_sourceId, ", amtId:", _id, ", pos:", param->m_position, ")");
    }
  }
}

void dsp_host_dual::globalParChg(const uint32_t _id, const nltools::msg::MacroControlChangedMessage& _msg)
{
  auto param = m_params.get_macro(_id);
  if(param->update_position(static_cast<float>(_msg.controlPosition)))
  {
    if(LOG_EDITS)
    {
      nltools::Log::info("mc_edit(pos:", param->m_position, ")");
    }
    globalModChain(param);
    if(m_layer_mode == LayerMode::Single)
    {
      localModChain(param);
    }
    else
    {
      for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
      {
        localModChain(layerId, param);
      }
    }
  }
}

void dsp_host_dual::globalParChg(const uint32_t _id, const nltools::msg::ModulateableParameterChangedMessage& _msg)
{
  const uint32_t macroId = getMacroId(_msg.sourceMacro);
  auto param = m_params.get_global_target(_id);
  bool aspect_update = param->update_source(getMacro(_msg.sourceMacro));
  if(aspect_update)
  {
    m_params.m_global.m_assignment.reassign(_id, macroId);
  }
  aspect_update |= param->update_amount(static_cast<float>(_msg.mcAmount));
  if(param->update_position(param->depolarize(static_cast<float>(_msg.controlPosition))))
  {
    param->update_modulation_aspects(m_params.get_macro(macroId)->m_position);
    param->m_scaled = scale(param->m_scaling, param->polarize(param->m_position));
    if(LOG_EDITS)
    {
      nltools::Log::info("global_target_edit(pos:", param->m_position, ", val:", param->m_scaled, ")");
      if(aspect_update)
      {
        nltools::Log::info("global_target_edit(mc:", macroId, ", amt:", param->m_amount, ")");
      }
    }
    globalTransition(param, m_edit_time.m_dx);
  }
  else if(aspect_update)
  {
    param->update_modulation_aspects(m_params.get_macro(macroId)->m_position);
    if(LOG_EDITS)
    {
      nltools::Log::info("global_target_edit(mc:", macroId, ", amt:", param->m_amount, ")");
    }
  }
  if(_id == static_cast<uint32_t>(C15::Parameters::Global_Modulateables::Split_Split_Point))
  {
    m_alloc.setSplitPoint(static_cast<uint32_t>(param->m_scaled));
  }
}

void dsp_host_dual::globalParChg(const uint32_t _id, const nltools::msg::UnmodulateableParameterChangedMessage& _msg)
{
  auto param = m_params.get_global_direct(_id);
  if(param->update_position(static_cast<float>(_msg.controlPosition)))
  {
    param->m_scaled = scale(param->m_scaling, param->m_position);
    if(LOG_EDITS)
    {
      nltools::Log::info("global_direct_edit(pos:", param->m_position, ", val:", param->m_scaled, ")");
    }
    if(_id == static_cast<uint32_t>(C15::Parameters::Global_Unmodulateables::Scale_Base_Key))
    {
      m_global.start_base_key(m_edit_time.m_dx.m_dx_slow, param->m_scaled);
    }
    else
    {
      globalTransition(param, m_edit_time.m_dx);
    }
  }
}

void dsp_host_dual::globalTimeChg(const uint32_t _id, const nltools::msg::UnmodulateableParameterChangedMessage& _msg)
{
  auto param = m_params.get_macro_time(_id);
  if(param->update_position(static_cast<float>(_msg.controlPosition)))
  {
    if(LOG_EDITS)
    {
      nltools::Log::info("mc_edit(time:", param->m_position, ")");
    }
    param->m_scaled = scale(param->m_scaling, param->m_position);
    updateTime(&param->m_dx, param->m_scaled);
  }
}

void dsp_host_dual::localParChg(const uint32_t _id, const nltools::msg::ModulateableParameterChangedMessage& _msg)
{
  const uint32_t layerId = getLayerId(_msg.voiceGroup), macroId = getMacroId(_msg.sourceMacro);
  auto param = m_params.get_local_target(layerId, _id);
  bool aspect_update = param->update_source(getMacro(_msg.sourceMacro));
  if(aspect_update)
  {
    m_params.m_layer[layerId].m_assignment.reassign(_id, macroId);
  }
  aspect_update |= param->update_amount(static_cast<float>(_msg.mcAmount));
  if(param->update_position(param->depolarize(static_cast<float>(_msg.controlPosition))))
  {
    param->update_modulation_aspects(m_params.get_macro(macroId)->m_position);
    param->m_scaled = scale(param->m_scaling, param->polarize(param->m_position));
    if(LOG_EDITS)
    {
      nltools::Log::info("local_target_edit(layer:", layerId, ", pos:", param->m_position, ", val:", param->m_scaled,
                         ")");
      if(aspect_update)
      {
        nltools::Log::info("local_target_edit(layer:", layerId, ", mc:", macroId, ", amt:", param->m_amount, ")");
      }
    }
    switch(static_cast<C15::Parameters::Local_Modulateables>(_id))
    {
      case C15::Parameters::Local_Modulateables::Unison_Detune:
      case C15::Parameters::Local_Modulateables::Mono_Grp_Glide:
        if(m_layer_mode != LayerMode::Split)
        {
          for(uint32_t lId = 0; lId < m_params.m_layer_count; lId++)
          {
            localTransition(lId, param, m_edit_time.m_dx);
          }
        }
        else
        {
          localTransition(layerId, param, m_edit_time.m_dx);
        }
        break;
      default:
        if(m_layer_mode == LayerMode::Single)
        {
          for(uint32_t lId = 0; lId < m_params.m_layer_count; lId++)
          {
            localTransition(lId, param, m_edit_time.m_dx);
          }
        }
        else
        {
          localTransition(layerId, param, m_edit_time.m_dx);
        }
        break;
    }
  }
  else if(aspect_update)
  {
    param->update_modulation_aspects(m_params.get_macro(macroId)->m_position);
    if(LOG_EDITS)
    {
      nltools::Log::info("local_target_edit(layer:", layerId, ", mc:", macroId, ", amt:", param->m_amount, ")");
    }
  }
}

void dsp_host_dual::localParChg(const uint32_t _id, const nltools::msg::UnmodulateableParameterChangedMessage& _msg)
{
  const uint32_t layerId = getLayerId(_msg.voiceGroup);
  auto param = m_params.get_local_direct(layerId, _id);
  if(param->update_position(static_cast<float>(_msg.controlPosition)))
  {
    param->m_scaled = scale(param->m_scaling, param->m_position);
    if(LOG_EDITS)
    {
      nltools::Log::info("local_direct_edit(layer:", layerId, ", pos:", param->m_position, ", val:", param->m_scaled,
                         ")");
    }
    switch(static_cast<C15::Parameters::Local_Unmodulateables>(_id))
    {
      case C15::Parameters::Local_Unmodulateables::Voice_Grp_Fade_From:
      case C15::Parameters::Local_Unmodulateables::Voice_Grp_Fade_Range:
        if(m_layer_mode == LayerMode::Layer)
        {
          evalVoiceFadeChg(layerId);
        }
        break;
      case C15::Parameters::Local_Unmodulateables::Unison_Phase:
      case C15::Parameters::Local_Unmodulateables::Unison_Pan:
        if(m_layer_mode != LayerMode::Split)
        {
          for(uint32_t lId = 0; lId < m_params.m_layer_count; lId++)
          {
            localTransition(lId, param, m_edit_time.m_dx);
          }
        }
        else
        {
          localTransition(layerId, param, m_edit_time.m_dx);
        }
        break;
      default:
        if(m_layer_mode == LayerMode::Single)
        {
          for(uint32_t lId = 0; lId < m_params.m_layer_count; lId++)
          {
            localTransition(lId, param, m_edit_time.m_dx);
          }
        }
        else
        {
          localTransition(layerId, param, m_edit_time.m_dx);
        }
        break;
    }
  }
}

void dsp_host_dual::localUnisonVoicesChg(const nltools::msg::UnmodulateableParameterChangedMessage& _msg)
{
  const uint32_t layerId = getLayerId(_msg.voiceGroup);
  auto param = m_params.get_local_unison_voices(getLayer(_msg.voiceGroup));

  if(param->update_position(static_cast<float>(_msg.controlPosition)))
  {
    if(LOG_EDITS)
    {
      nltools::Log::info("unison_voices_edit(layer:", layerId, ", pos:", param->m_position, ")");
    }

    // application now via fade point
    m_fade.muteAndDo([&] {
      // apply (preloaded) unison change
      m_alloc.setUnison(layerId, param->m_position, m_layer_mode, m_layer_mode);
      // apply reset to affected poly compoments
      m_poly[layerId].resetEnvelopes();
      m_poly[layerId].m_uVoice = m_alloc.m_unison - 1;
      m_poly[layerId].m_key_active = 0;
      if(m_layer_mode != LayerMode::Split)
      {
        // apply reset to other poly components (when not in split mode)
        const uint32_t lId = 1 - layerId;
        m_poly[lId].resetEnvelopes();
        m_poly[lId].m_uVoice = m_alloc.m_unison - 1;
        m_poly[lId].m_key_active = 0;
      }
    });
  }
}

void dsp_host_dual::localMonoEnableChg(const nltools::msg::UnmodulateableParameterChangedMessage& _msg)
{
  const uint32_t layerId = getLayerId(_msg.voiceGroup);
  auto param = m_params.get_local_mono_enable(getLayer(_msg.voiceGroup));
  if(param->update_position(static_cast<float>(_msg.controlPosition)))
  {
    if(LOG_EDITS)
    {
      nltools::Log::info("mono_enable_edit(layer:", layerId, ", pos:", param->m_position, ")");
    }
    param->m_scaled = scale(param->m_scaling, param->m_position);
    // application now via fade point
    m_fade.muteAndDo([&] {
      // apply (preloaded) mono change
      m_alloc.setMonoEnable(layerId, param->m_scaled, m_layer_mode);
      // apply reset to affected poly compoments
      m_poly[layerId].resetEnvelopes();
      m_poly[layerId].m_key_active = 0;
      if(m_layer_mode != LayerMode::Split)
      {
        // apply reset to other poly components (when not in split mode)
        const uint32_t lId = 1 - layerId;
        m_poly[lId].resetEnvelopes();
        m_poly[lId].m_key_active = 0;
      }
    });
  }
}

void dsp_host_dual::localMonoPriorityChg(const nltools::msg::UnmodulateableParameterChangedMessage& _msg)
{
  const uint32_t layerId = getLayerId(_msg.voiceGroup);
  auto param = m_params.get_local_mono_priority(getLayer(_msg.voiceGroup));
  if(param->update_position(static_cast<float>(_msg.controlPosition)))
  {
    if(LOG_EDITS)
    {
      nltools::Log::info("mono_priority_edit(layer:", layerId, ", pos:", param->m_position, ")");
    }
    param->m_scaled = scale(param->m_scaling, param->m_position);
    m_alloc.setMonoPriority(layerId, param->m_scaled, m_layer_mode);
  }
}
void dsp_host_dual::localMonoLegatoChg(const nltools::msg::UnmodulateableParameterChangedMessage& _msg)
{
  const uint32_t layerId = getLayerId(_msg.voiceGroup);
  auto param = m_params.get_local_mono_legato(getLayer(_msg.voiceGroup));
  if(param->update_position(static_cast<float>(_msg.controlPosition)))
  {
    if(LOG_EDITS)
    {
      nltools::Log::info("mono_leagto_edit(layer:", layerId, ", pos:", param->m_position, ")");
    }
    param->m_scaled = scale(param->m_scaling, param->m_position);
    m_alloc.setMonoLegato(layerId, param->m_scaled, m_layer_mode);
  }
}

void dsp_host_dual::onSettingEditTime(const float _position)
{
  // inconvenient clamping of odd position range ...
  if(m_edit_time.update_position(_position < 0.0f ? 0.0f : _position > 1.0f ? 1.0f : _position))
  {
    if(LOG_SETTINGS)
    {
      nltools::Log::info("edit_time(pos:", m_edit_time.m_position, ", raw:", _position, ")");
    }
    updateTime(&m_edit_time.m_dx, scale(m_edit_time.m_scaling, m_edit_time.m_position));
  }
}

void dsp_host_dual::onSettingTransitionTime(const float _position)
{
  // inconvenient clamping of odd position range ...
  if(m_transition_time.update_position(_position < 0.0f ? 0.0f : _position > 1.0f ? 1.0f : _position))
  {
    if(LOG_SETTINGS)
    {
      nltools::Log::info("transition_time(pos:", m_transition_time.m_position, ", raw:", _position, ")");
    }
    updateTime(&m_transition_time.m_dx, scale(m_transition_time.m_scaling, m_transition_time.m_position));
  }
}

void dsp_host_dual::onSettingNoteShift(const float _shift)
{
  m_poly[0].m_note_shift = m_poly[1].m_note_shift = _shift;
  if(LOG_SETTINGS)
  {
    nltools::Log::info("note_shift:", _shift);
  }
}

void dsp_host_dual::onSettingGlitchSuppr(const bool _enabled)
{
  m_glitch_suppression = _enabled;
  if(LOG_SETTINGS)
  {
    nltools::Log::info("glitch_suppression:", _enabled);
  }
}

void dsp_host_dual::onSettingTuneReference(const float _position)
{
  // inconvenient clamping of odd position range ...
  if(m_reference.update_position(_position < 0.0f ? 0.0f : _position > 1.0f ? 1.0f : _position))
  {
    m_reference.m_scaled = scale(m_reference.m_scaling, m_reference.m_position);
    m_global.update_tone_frequency(m_reference.m_scaled);
    if(LOG_SETTINGS)
    {
      nltools::Log::info("tune_reference:", _position);
    }
  }
}

void dsp_host_dual::onSettingInitialSinglePreset()
{
  if(LOG_RECALL)
  {
    nltools::Log::info("recallInitialSinglePreset(@", m_clock.m_index, ")");
  }
  m_layer_mode = LayerMode::Single;
  auto unison = m_params.get_local_unison_voices(C15::Properties::LayerId::I);
  unison->update_position(unison->m_initial);
  if(LOG_RESET)
  {
    nltools::Log::info("recall single voice reset");
  }
  m_alloc.setUnison(0, unison->m_position, m_layer_mode, m_layer_mode);
  m_params.m_global.m_assignment.reset();
  const uint32_t uVoice = m_alloc.m_unison - 1;
  for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
  {
    m_poly[layerId].resetEnvelopes();
    m_poly[layerId].m_uVoice = uVoice;
    m_poly[layerId].m_key_active = 0;
    m_params.m_layer[layerId].m_assignment.reset();
  }
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: hw_sources:");
  }
  for(uint32_t i = 0; i < m_params.m_global.m_source_count; i++)
  {
    auto param = m_params.get_hw_src(i);
    //param->update_behavior(C15::Properties::HW_Return_Behavior::Zero);
    param->update_position(param->m_initial);
  }
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: hw_amounts:");
  }
  for(uint32_t i = 0; i < m_params.m_global.m_amount_count; i++)
  {
    auto param = m_params.get_hw_amt(i);
    param->update_position(param->m_initial);
  }
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: macros:");
  }
  for(uint32_t i = 0; i < m_params.m_global.m_macro_count; i++)
  {
    auto param = m_params.get_macro(i);
    param->update_position(param->m_initial);
  }
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: global unmodulateables:");
  }
  for(uint32_t i = 0; i < m_params.m_global.m_direct_count; i++)
  {
    auto param = m_params.get_global_direct(i);
    param->update_position(param->m_initial);
    param->m_scaled = scale(param->m_scaling, param->m_position);
    globalTransition(param, m_transition_time.m_dx);
  }
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: global modulateables:");
  }
  for(uint32_t i = 0; i < m_params.m_global.m_target_count; i++)
  {
    auto param = m_params.get_global_target(i);
    param->update_source(C15::Parameters::Macro_Controls::None);
    param->update_amount(0.0f);
    param->update_position(param->m_initial);
    param->update_modulation_aspects(0.0f);
    param->m_scaled = scale(param->m_scaling, param->m_position);
    globalTransition(param, m_transition_time.m_dx);
  }
  // maybe, setting both voice groups resolves missing split sound?
  for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
  {
    if(LOG_RECALL)
    {
      nltools::Log::info("recall: local unmodulateables:");
    }
    for(uint32_t i = 0; i < m_params.m_layer[0].m_direct_count; i++)
    {
      auto param = m_params.get_local_direct(layerId, i);
      param->update_position(param->m_initial);
      param->m_scaled = scale(param->m_scaling, param->m_position);
      localTransition(layerId, param, m_transition_time.m_dx);
    }
    if(LOG_RECALL)
    {
      nltools::Log::info("recall: local modulateables:");
    }
    for(uint32_t i = 0; i < m_params.m_layer[0].m_target_count; i++)
    {
      auto param = m_params.get_local_target(layerId, i);
      param->update_source(C15::Parameters::Macro_Controls::None);
      param->update_amount(0.0f);
      param->update_position(param->depolarize(param->m_initial));
      param->update_modulation_aspects(m_params.get_macro(static_cast<uint32_t>(param->m_source))->m_position);
      param->m_scaled = scale(param->m_scaling, param->polarize(param->m_position));
      localTransition(layerId, param, m_transition_time.m_dx);
    }
  }
}

uint32_t dsp_host_dual::onSettingToneToggle()
{
  m_tone_state = (m_tone_state + 1) % 3;
  m_fade.muteAndDo([&] { m_global.update_tone_mode(m_tone_state); });
  return m_tone_state;
}

void dsp_host_dual::render()
{
  m_sample_counter += SAMPLE_COUNT;
  m_fade.evalTaskStatus();

  if(m_fade.isMuted())
  {
    m_mainOut_L = m_mainOut_R = 0.0f;
    m_fade.onMuteSampleRendered();
    return;
  }

  m_clock.render();

  // slow rendering
  if(m_clock.m_slow)
  {
    // render components
    m_global.render_slow();
    m_poly[0].render_slow();
    m_poly[1].render_slow();
    m_mono[0].render_slow();
    m_mono[1].render_slow();
  }
  // fast rendering
  if(m_clock.m_fast)
  {
    // render components
    m_global.render_fast();
    m_poly[0].render_fast();
    m_poly[1].render_fast();
    m_mono[0].render_fast();
    m_mono[1].render_fast();
  }
  // audio rendering (always) -- temporary soundgenerator patching
  const float mute = m_fade.getValue();
  // - audio dsp poly - first stage - both layers (up to Output Mixer)
  m_poly[0].render_audio(mute);
  m_poly[1].render_audio(mute);
  // - audio dsp mono - each layer with separate sends - left, right)

  m_mono[0].render_audio(m_poly[0].m_send_self_l + m_poly[1].m_send_other_l,
                         m_poly[0].m_send_self_r + m_poly[1].m_send_other_r, m_poly[0].getVoiceGroupVolume());
  m_mono[1].render_audio(m_poly[0].m_send_other_l + m_poly[1].m_send_self_l,
                         m_poly[0].m_send_other_r + m_poly[1].m_send_self_r, m_poly[1].getVoiceGroupVolume());
  // audio dsp poly - second stage - both layers (FB Mixer)
  m_poly[0].render_feedback(m_z_layers[1]);  // pass other layer's signals as arg
  m_poly[1].render_feedback(m_z_layers[0]);  // pass other layer's signals as arg
  // - audio dsp global - main out: combine layers, apply test_tone and soft clip
  m_global.render_audio(m_mono[0].m_out_l + m_mono[1].m_out_l, m_mono[0].m_out_r + m_mono[1].m_out_r);
  // - final: main out, output mute
  m_mainOut_L = m_global.m_out_l * mute;
  m_mainOut_R = m_global.m_out_r * mute;
}

void dsp_host_dual::reset()
{
  for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
  {
    m_z_layers[layerId].reset();
    m_poly[layerId].resetDSP();
    m_mono[layerId].resetDSP();
  }
  m_global.resetDSP();
  m_mainOut_L = m_mainOut_R = 0.0f;
  if(LOG_RESET)
  {
    nltools::Log::info("DSP has been reset.");
  }
}

C15::Properties::HW_Return_Behavior dsp_host_dual::getBehavior(const ReturnMode _mode)
{
  switch(_mode)
  {
    case ReturnMode::None:
      return C15::Properties::HW_Return_Behavior::Stay;
    case ReturnMode::Zero:
      return C15::Properties::HW_Return_Behavior::Zero;
    case ReturnMode::Center:
      return C15::Properties::HW_Return_Behavior::Center;
    default:
      return C15::Properties::HW_Return_Behavior::Stay;
  }
}

C15::Properties::HW_Return_Behavior dsp_host_dual::getBehavior(const RibbonReturnMode _mode)
{
  switch(_mode)
  {
    case RibbonReturnMode::STAY:
      return C15::Properties::HW_Return_Behavior::Stay;
    case RibbonReturnMode::RETURN:
      return C15::Properties::HW_Return_Behavior::Center;
    default:
      return C15::Properties::HW_Return_Behavior::Stay;
  }
}

C15::Parameters::Macro_Controls dsp_host_dual::getMacro(const MacroControls _mc)
{
  switch(_mc)
  {
    case MacroControls::NONE:
      return C15::Parameters::Macro_Controls::None;
    case MacroControls::MC1:
      return C15::Parameters::Macro_Controls::MC_A;
    case MacroControls::MC2:
      return C15::Parameters::Macro_Controls::MC_B;
    case MacroControls::MC3:
      return C15::Parameters::Macro_Controls::MC_C;
    case MacroControls::MC4:
      return C15::Parameters::Macro_Controls::MC_D;
    case MacroControls::MC5:
      return C15::Parameters::Macro_Controls::MC_E;
    case MacroControls::MC6:
      return C15::Parameters::Macro_Controls::MC_F;
    default:
      return C15::Parameters::Macro_Controls::None;
  }
  // maybe, a simple static_cast would be sufficient ...
}

uint32_t dsp_host_dual::getMacroId(const MacroControls _mc)
{
  return static_cast<uint32_t>(_mc);  // assuming idential MC enumeration
}

C15::Properties::LayerId dsp_host_dual::getLayer(const VoiceGroup _vg)
{
  switch(_vg)
  {
    case VoiceGroup::I:
      return C15::Properties::LayerId::I;
    case VoiceGroup::II:
      return C15::Properties::LayerId::II;
    default:
      return C15::Properties::LayerId::I;
  }
}

uint32_t dsp_host_dual::getLayerId(const VoiceGroup _vg)
{
  switch(_vg)
  {
    case VoiceGroup::I:
      return 0;
    case VoiceGroup::II:
      return 1;
    default:
      return 0;
  }
}

void dsp_host_dual::keyDown(const float _vel)
{
  if(m_alloc.keyDown(m_key_pos, _vel, m_layer_mode))
  {
    if(LOG_KEYS)
    {
      nltools::Log::info("key_down(pos:", m_key_pos, ", vel:", _vel, ", unison:", m_alloc.m_unison, ")");
    }
    for(auto event = m_alloc.m_traversal.first(); m_alloc.m_traversal.running(); event = m_alloc.m_traversal.next())
    {
      if(m_poly[event->m_localIndex].keyDown(event))
      {
        // mono legato
        m_mono[event->m_localIndex].keyDown(event);
      }
      if(LOG_KEYS_POLY)
      {
        nltools::Log::info("key_down_poly(group:", event->m_localIndex, "voice:", event->m_voiceId,
                           ", unisonIndex:", event->m_unisonIndex, ", stolen:", event->m_stolen,
                           ", tune:", event->m_tune, ", velocity:", event->m_velocity, ")");
        nltools::Log::info("key_details(active:", event->m_active, ", trigger_env:", event->m_trigger_env,
                           ", trigger_glide:", event->m_trigger_glide, ")");
      }
    }
  }
  else if(LOG_FAIL)

  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "keyDown(pos:", m_key_pos, ") failed!");
  }
}

void dsp_host_dual::keyUp(const float _vel)
{
  if(m_alloc.keyUp(m_key_pos, _vel))
  {
    if(LOG_KEYS)
    {
      nltools::Log::info("key_up(pos:", m_key_pos, ", vel:", _vel, ", unison:", m_alloc.m_unison, ")");
    }
    for(auto event = m_alloc.m_traversal.first(); m_alloc.m_traversal.running(); event = m_alloc.m_traversal.next())
    {
      m_poly[event->m_localIndex].keyUp(event);
      if(LOG_KEYS_POLY)
      {
        nltools::Log::info("key_up_poly(group:", event->m_localIndex, "voice:", event->m_voiceId,
                           ", tune:", event->m_tune, ", velocity:", event->m_velocity, ")");
        nltools::Log::info("key_details(active:", event->m_active, ", trigger_env:", event->m_trigger_env,
                           ", trigger_glide:", event->m_trigger_glide, ")");
      }
    }
  }
  else if(LOG_FAIL)
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "keyUp(pos:", m_key_pos, ") failed!");
  }
}

float dsp_host_dual::scale(const Scale_Aspect _scl, float _value)
{
  float result = 0.0f;
  switch(_scl.m_id)
  {
    case C15::Properties::SmootherScale::None:
      break;
    case C15::Properties::SmootherScale::Linear:
      result = _scl.m_offset + (_scl.m_factor * _value);
      break;
    case C15::Properties::SmootherScale::Parabolic:
      result = _scl.m_offset + (_scl.m_factor * _value * std::abs(_value));
      break;
    case C15::Properties::SmootherScale::Cubic:
      result = _scl.m_offset + (_scl.m_factor * _value * _value * _value);
      break;
    case C15::Properties::SmootherScale::S_Curve:
      _value = (2.0f * (1.0f - _value)) - 1.0f;
      result = _scl.m_offset + (_scl.m_factor * ((_value * _value * _value * -0.25f) + (_value * 0.75f) + 0.5f));
      break;
    case C15::Properties::SmootherScale::Expon_Gain:
      result = m_convert.eval_level(_scl.m_offset + (_scl.m_factor * _value));
      break;
    case C15::Properties::SmootherScale::Expon_Osc_Pitch:
      result = m_convert.eval_osc_pitch(_scl.m_offset + (_scl.m_factor * _value));
      break;
    case C15::Properties::SmootherScale::Expon_Lin_Pitch:
      result = m_convert.eval_lin_pitch(_scl.m_offset + (_scl.m_factor * _value));
      break;
    case C15::Properties::SmootherScale::Expon_Shaper_Drive:
      result = (m_convert.eval_level(_value * _scl.m_factor) * _scl.m_offset) - _scl.m_offset;
      break;
    case C15::Properties::SmootherScale::Expon_Mix_Drive:
      result = _scl.m_offset * m_convert.eval_level(_scl.m_factor * _value);
      break;
    case C15::Properties::SmootherScale::Expon_Env_Time:
      result = m_convert.eval_time((_value * _scl.m_factor * 104.0781f) + _scl.m_offset);
      break;
  }
  return result;
}

void dsp_host_dual::updateHW(const uint32_t _id, const float _raw)
{
  auto source = m_params.get_hw_src(_id);
  float value = _raw * m_norm_hw;
  if(source->m_behavior == C15::Properties::HW_Return_Behavior::Center)
  {
    value = (2.0f * value) - 1.0f;  // make return_to_center sources bipolar
  }
  const float inc = value - source->m_position;
  source->m_position = value;
  hwModChain(source, _id, inc);
}

void dsp_host_dual::updateTime(Time_Aspect* _param, const float _ms)
{
  m_time.update_ms(_ms);
  _param->m_dx_audio = m_time.m_dx_audio;
  _param->m_dx_fast = m_time.m_dx_fast;
  _param->m_dx_slow = m_time.m_dx_slow;
  if(LOG_TIMES)
  {
    nltools::Log::info("time_update(ms:", _ms, ", dx:", m_time.m_dx_audio, m_time.m_dx_fast, m_time.m_dx_slow, ")");
  }
}

void dsp_host_dual::hwModChain(HW_Src_Param* _src, const uint32_t _id, const float _inc)
{
  if(LOG_HW)
  {
    nltools::Log::info("hw_move(id:", _id, ", pos:", _src->m_position, ")");
  }
  if(_src->m_behavior == C15::Properties::HW_Return_Behavior::Stay)
  {
    for(uint32_t macroId = 1; macroId < m_params.m_global.m_macro_count; macroId++)
    {
      const uint32_t amountIndex = _src->m_offset + macroId - 1;
      auto amount = m_params.get_hw_amt(amountIndex);
      if(amount->m_position != 0.0f)
      {
        auto macro = m_params.get_macro(macroId);
        macro->m_position = _src->m_position;
        if(m_layer_mode == LayerMode::Single)
        {
          if(LOG_HW)
          {
            nltools::Log::info("hw_nonreturn_to_mc(mode: single, srcId:", _id, ", mcId:", macroId,
                               ", amtId:", amountIndex, ", amount:", amount->m_position, ", value:", macro->m_position,
                               ")");
          }
          localModChain(macro);
        }
        else
        {
          if(LOG_HW)
          {
            nltools::Log::info("hw_nonreturn_to_mc(mode: dual, srcId:", _id, ", mcId:", macroId,
                               ", amtId:", amountIndex, ", amount:", amount->m_position, ", value:", macro->m_position,
                               ")");
          }
          for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
          {
            localModChain(layerId, macro);
          }
        }
        globalModChain(macro);
      }
    }
  }
  else
  {
    for(uint32_t macroId = 1; macroId < m_params.m_global.m_macro_count; macroId++)
    {
      const uint32_t amountIndex = _src->m_offset + macroId - 1;
      auto amount = m_params.get_hw_amt(amountIndex);
      if(amount->m_position != 0.0f)
      {
        auto amount = m_params.get_hw_amt(amountIndex);
        if(amount->m_position != 0.0f)
        {
          auto macro = m_params.get_macro(macroId);
          macro->m_unclipped = macro->m_position + (_inc * amount->m_position);
          const float clipped
              = macro->m_unclipped < 0.0f ? 0.0f : macro->m_unclipped > 1.0f ? 1.0f : macro->m_unclipped;
          if(macro->m_position != clipped)
          {
            macro->m_position = clipped;
            if(m_layer_mode == LayerMode::Single)
            {
              if(LOG_HW)
              {
                nltools::Log::info("hw_return_to_mc(mode: single, srcId:", _id, ", mcId:", macroId,
                                   ", amtId:", amountIndex, ", amount:", amount->m_position,
                                   ", value:", macro->m_position, ")");
              }
              localModChain(macro);
            }
            else
            {
              if(LOG_HW)
              {
                nltools::Log::info("hw_return_to_mc(mode: dual, srcId:", _id, ", mcId:", macroId,
                                   ", amtId:", amountIndex, ", amount:", amount->m_position,
                                   ", value:", macro->m_position, ")");
              }
              for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
              {
                localModChain(layerId, macro);
              }
            }
            globalModChain(macro);
          }
        }
      }
    }
  }
}

void dsp_host_dual::globalModChain(Macro_Param* _mc)
{
  for(auto param = m_params.globalChainFirst(_mc->m_id); m_params.globalChainRunning();
      param = m_params.globalChainNext())
  {
    if(param->modulate(_mc->m_position))
    {
      const float clipped = param->m_unclipped < 0.0f ? 0.0f : param->m_unclipped > 1.0f ? 1.0f : param->m_unclipped;
      if(param->m_position != clipped)
      {
        param->m_position = clipped;
        param->m_scaled = scale(param->m_scaling, param->polarize(clipped));
        globalTransition(param, _mc->m_time.m_dx);
        if(param->m_splitpoint)
        {
          m_alloc.setSplitPoint(static_cast<uint32_t>(param->m_scaled));
        }
      }
    }
  }
}

void dsp_host_dual::localModChain(Macro_Param* _mc)
{
  for(auto param = m_params.localChainFirst(0, _mc->m_id); m_params.localChainRunning(0);
      param = m_params.localChainNext(0))
  {
    if(param->modulate(_mc->m_position))
    {
      const float clipped = param->m_unclipped < 0.0f ? 0.0f : param->m_unclipped > 1.0f ? 1.0f : param->m_unclipped;
      if(param->m_position != clipped)
      {
        param->m_position = clipped;
        param->m_scaled = scale(param->m_scaling, param->polarize(clipped));
        localTransition(0, param, _mc->m_time.m_dx);
        localTransition(1, param, _mc->m_time.m_dx);
      }
    }
  }
}

void dsp_host_dual::localModChain(const uint32_t _layer, Macro_Param* _mc)
{
  for(auto param = m_params.localChainFirst(_layer, _mc->m_id); m_params.localChainRunning(_layer);
      param = m_params.localChainNext(_layer))
  {
    if(param->modulate(_mc->m_position))
    {
      const float clipped = param->m_unclipped < 0.0f ? 0.0f : param->m_unclipped > 1.0f ? 1.0f : param->m_unclipped;
      if(param->m_position != clipped)
      {
        param->m_position = clipped;
        param->m_scaled = scale(param->m_scaling, param->polarize(clipped));
        localTransition(_layer, param, _mc->m_time.m_dx);
      }
    }
  }
}

void dsp_host_dual::globalTransition(const Target_Param* _param, const Time_Aspect _time)
{
  float dx = 0.0f, dest = _param->m_scaled;
  bool fail = false;
  switch(_param->m_render.m_section)
  {
    case C15::Descriptors::SmootherSection::None:
      // no smooother - no transition - no fail
      break;
    case C15::Descriptors::SmootherSection::Global:
      switch(_param->m_render.m_clock)
      {
        case C15::Descriptors::SmootherClock::Sync:
          m_global.start_sync(_param->m_render.m_index, dest);
          break;
        case C15::Descriptors::SmootherClock::Audio:
          dx = _time.m_dx_audio;
          m_global.start_audio(_param->m_render.m_index, dx, dest);
          break;
        case C15::Descriptors::SmootherClock::Fast:
          dx = _time.m_dx_fast;
          m_global.start_fast(_param->m_render.m_index, dx, dest);
          break;
        case C15::Descriptors::SmootherClock::Slow:
          dx = _time.m_dx_slow;
          m_global.start_slow(_param->m_render.m_index, dx, dest);
          break;
      }
      break;
    default:
      fail = true;
      break;
  }
  if(LOG_TRANSITIONS && !fail)
  {
    nltools::Log::info("global_transition(section:", static_cast<int>(_param->m_render.m_section),
                       ", index:", _param->m_render.m_index, ", clock:", static_cast<int>(_param->m_render.m_clock),
                       ", dx:", dx, ", dest:", dest, ")");
  }
  else if(LOG_FAIL && fail)
  {
    nltools::Log::warning(
        __PRETTY_FUNCTION__, "failed to start global_transition(section:", static_cast<int>(_param->m_render.m_section),
        ", index:", _param->m_render.m_index, ", clock:", static_cast<int>(_param->m_render.m_clock), ")");
  }
}

void dsp_host_dual::globalTransition(const Direct_Param* _param, const Time_Aspect _time)
{
  float dx = 0.0f, dest = _param->m_scaled;
  bool fail = false;
  switch(_param->m_render.m_section)
  {
    case C15::Descriptors::SmootherSection::None:
      // no smooother - no transition - no fail
      break;
    case C15::Descriptors::SmootherSection::Global:
      switch(_param->m_render.m_clock)
      {
        case C15::Descriptors::SmootherClock::Sync:
          m_global.start_sync(_param->m_render.m_index, dest);
          break;
        case C15::Descriptors::SmootherClock::Audio:
          dx = _time.m_dx_audio;
          m_global.start_audio(_param->m_render.m_index, dx, dest);
          break;
        case C15::Descriptors::SmootherClock::Fast:
          dx = _time.m_dx_fast;
          m_global.start_fast(_param->m_render.m_index, dx, dest);
          break;
        case C15::Descriptors::SmootherClock::Slow:
          dx = _time.m_dx_slow;
          m_global.start_slow(_param->m_render.m_index, dx, dest);
          break;
      }
      break;
    default:
      fail = true;
      break;
  }
  if(LOG_TRANSITIONS && !fail)
  {
    nltools::Log::info("global_transition(section:", static_cast<int>(_param->m_render.m_section),
                       ", index:", _param->m_render.m_index, ", clock:", static_cast<int>(_param->m_render.m_clock),
                       ", dx:", dx, ", dest:", dest, ")");
  }
  if(LOG_FAIL && fail)
  {
    nltools::Log::warning(
        __PRETTY_FUNCTION__, "failed to start global_transition(section:", static_cast<int>(_param->m_render.m_section),
        ", index:", _param->m_render.m_index, ", clock:", static_cast<int>(_param->m_render.m_clock), ")");
  }
}

void dsp_host_dual::localTransition(const uint32_t _layer, const Direct_Param* _param, const Time_Aspect _time)
{
  float dx = 0.0f, dest = _param->m_scaled;
  bool fail = false;
  switch(_param->m_render.m_section)
  {
    case C15::Descriptors::SmootherSection::None:
      // no smooother - no transition - no fail
      break;
    case C15::Descriptors::SmootherSection::Poly:
      switch(_param->m_render.m_clock)
      {
        case C15::Descriptors::SmootherClock::Sync:
          m_poly[_layer].start_sync(_param->m_render.m_index, dest);
          break;
        case C15::Descriptors::SmootherClock::Audio:
          dx = _time.m_dx_audio;
          m_poly[_layer].start_audio(_param->m_render.m_index, dx, dest);
          break;
        case C15::Descriptors::SmootherClock::Fast:
          dx = _time.m_dx_fast;
          m_poly[_layer].start_fast(_param->m_render.m_index, dx, dest);
          break;
        case C15::Descriptors::SmootherClock::Slow:
          dx = _time.m_dx_slow;
          m_poly[_layer].start_slow(_param->m_render.m_index, dx, dest);
          break;
      }
      break;
    case C15::Descriptors::SmootherSection::Mono:
      switch(_param->m_render.m_clock)
      {
        case C15::Descriptors::SmootherClock::Sync:
          m_mono[_layer].start_sync(_param->m_render.m_index, dest);
          break;
        case C15::Descriptors::SmootherClock::Audio:
          dx = _time.m_dx_audio;
          m_mono[_layer].start_audio(_param->m_render.m_index, dx, dest);
          break;
        case C15::Descriptors::SmootherClock::Fast:
          dx = _time.m_dx_fast;
          m_mono[_layer].start_fast(_param->m_render.m_index, dx, dest);
          break;
        case C15::Descriptors::SmootherClock::Slow:
          dx = _time.m_dx_slow;
          m_mono[_layer].start_slow(_param->m_render.m_index, dx, dest);
          break;
      }
      break;
    default:
      fail = true;
      break;
  }
  if(LOG_TRANSITIONS && !fail)
  {
    nltools::Log::info("local_transition(layer:", _layer, ", section:", static_cast<int>(_param->m_render.m_section),
                       ", index:", _param->m_render.m_index, ", clock:", static_cast<int>(_param->m_render.m_clock),
                       ", dx:", dx, ", dest:", dest, ")");
  }
  if(LOG_FAIL && fail)
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "failed to start local_transition(layer:", _layer,
                          "section:", static_cast<int>(_param->m_render.m_section),
                          ", index:", _param->m_render.m_index, ", clock:", static_cast<int>(_param->m_render.m_clock),
                          ")");
  }
}
void dsp_host_dual::localTransition(const uint32_t _layer, const Target_Param* _param, const Time_Aspect _time)
{
  float dx = 0.0f, dest = _param->m_scaled;
  bool fail = false;
  switch(_param->m_render.m_section)
  {
    case C15::Descriptors::SmootherSection::None:
      // no smooother - no transition - no fail
      break;
    case C15::Descriptors::SmootherSection::Poly:
      switch(_param->m_render.m_clock)
      {
        case C15::Descriptors::SmootherClock::Sync:
          m_poly[_layer].start_sync(_param->m_render.m_index, dest);
          break;
        case C15::Descriptors::SmootherClock::Audio:
          dx = _time.m_dx_audio;
          m_poly[_layer].start_audio(_param->m_render.m_index, dx, dest);
          break;
        case C15::Descriptors::SmootherClock::Fast:
          dx = _time.m_dx_fast;
          m_poly[_layer].start_fast(_param->m_render.m_index, dx, dest);
          break;
        case C15::Descriptors::SmootherClock::Slow:
          dx = _time.m_dx_slow;
          m_poly[_layer].start_slow(_param->m_render.m_index, dx, dest);
          break;
      }
      break;
    case C15::Descriptors::SmootherSection::Mono:
      switch(_param->m_render.m_clock)
      {
        case C15::Descriptors::SmootherClock::Sync:
          m_mono[_layer].start_sync(_param->m_render.m_index, dest);
          break;
        case C15::Descriptors::SmootherClock::Audio:
          dx = _time.m_dx_audio;
          m_mono[_layer].start_audio(_param->m_render.m_index, dx, dest);
          break;
        case C15::Descriptors::SmootherClock::Fast:
          dx = _time.m_dx_fast;
          m_mono[_layer].start_fast(_param->m_render.m_index, dx, dest);
          break;
        case C15::Descriptors::SmootherClock::Slow:
          dx = _time.m_dx_slow;
          m_mono[_layer].start_slow(_param->m_render.m_index, dx, dest);
          break;
      }
      break;
    default:
      fail = true;
      break;
  }
  if(LOG_TRANSITIONS && !fail)
  {
    nltools::Log::info("local_transition(layer:", _layer, ", section:", static_cast<int>(_param->m_render.m_section),
                       ", index:", _param->m_render.m_index, ", clock:", static_cast<int>(_param->m_render.m_clock),
                       ", dx:", dx, ", dest:", dest, ")");
  }
  if(LOG_FAIL && fail)
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "failed to start local_transition(layer:", _layer,
                          "section:", static_cast<int>(_param->m_render.m_section),
                          ", index:", _param->m_render.m_index, ", clock:", static_cast<int>(_param->m_render.m_clock),
                          ")");
  }
}

bool dsp_host_dual::evalPolyChg(const C15::Properties::LayerId _layerId,
                                const nltools::msg::ParameterGroups::UnmodulateableParameter& _unisonVoices,
                                const nltools::msg::ParameterGroups::UnmodulateableParameter& _monoEnable)
{
  const uint32_t layerId = static_cast<uint32_t>(_layerId);
  auto unison_voices = m_params.get_local_unison_voices(_layerId);
  bool unisonChanged = unison_voices->update_position(static_cast<float>(_unisonVoices.controlPosition));
  auto mono_enable = m_params.get_local_mono_enable(_layerId);
  bool monoChanged = mono_enable->update_position(static_cast<float>(_monoEnable.controlPosition));
  return unisonChanged || monoChanged;
}

void dsp_host_dual::evalVoiceFadeChg(const uint32_t _layer)
{
  if(VOICE_FADE_INTERPOLATION)
  {
    m_poly[_layer].evalVoiceFadeInterpolated(
        m_params
            .get_local_direct(_layer,
                              static_cast<uint32_t>(C15::Parameters::Local_Unmodulateables::Voice_Grp_Fade_From))
            ->m_scaled,
        m_params
            .get_local_direct(_layer,
                              static_cast<uint32_t>(C15::Parameters::Local_Unmodulateables::Voice_Grp_Fade_Range))
            ->m_scaled);
  }
  else
  {
    m_poly[_layer].evalVoiceFade(
        m_params
            .get_local_direct(_layer,
                              static_cast<uint32_t>(C15::Parameters::Local_Unmodulateables::Voice_Grp_Fade_From))
            ->m_scaled,
        m_params
            .get_local_direct(_layer,
                              static_cast<uint32_t>(C15::Parameters::Local_Unmodulateables::Voice_Grp_Fade_Range))
            ->m_scaled);
  }
}

void dsp_host_dual::recallSingle(const nltools::msg::SinglePresetMessage& _msg)
{
  auto oldLayerMode = std::exchange(m_layer_mode, LayerMode::Single);
  auto layerChanged = oldLayerMode != m_layer_mode;

  if(LOG_RECALL)
  {
    nltools::Log::info("recallSingle(@", m_clock.m_index, ")");
  }
  auto msg = &_msg;
  // update unison and mono groups
  auto polyChanged = evalPolyChg(C15::Properties::LayerId::I, msg->unison.unisonVoices, msg->mono.monoEnable);
  // reset detection
  if(layerChanged || polyChanged)
  {
    if(LOG_RESET)
    {
      nltools::Log::info("recall single voice reset");
    }
    m_alloc.setUnison(0, m_params.get_local_unison_voices(C15::Properties::LayerId::I)->m_position, oldLayerMode,
                      m_layer_mode);
    m_alloc.setMonoEnable(0, m_params.get_local_mono_enable(C15::Properties::LayerId::I)->m_position, m_layer_mode);
    const uint32_t uVoice = m_alloc.m_unison - 1;
    for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
    {
      m_poly[layerId].resetEnvelopes();
      m_poly[layerId].m_uVoice = uVoice;
      m_poly[layerId].m_key_active = 0;
    }
  }
  // reset
  m_params.m_global.m_assignment.reset();
  for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
  {
    // macro assignments
    m_params.m_layer[layerId].m_assignment.reset();
    // voice fade
    m_poly[layerId].resetVoiceFade();
  }
  // global updates: hw sources
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: hw sources:");
  }
  for(uint32_t i = 0; i < msg->hwsources.size(); i++)
  {
    globalParRcl(msg->hwsources[i]);
  }
  // global updates: hw amounts
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: hw amounts:");
  }
  for(uint32_t i = 0; i < msg->hwamounts.size(); i++)
  {
    globalParRcl(msg->hwamounts[i]);
  }
  // global updates: macros
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: macros:");
  }
  for(uint32_t i = 0; i < msg->macros.size(); i++)
  {
    globalParRcl(msg->macros[i]);
  }
  for(uint32_t i = 0; i < msg->macrotimes.size(); i++)
  {
    globalTimeRcl(msg->macrotimes[i]);
  }
  // global updates: parameters
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: global params:");
  }
  globalParRcl(msg->master.volume);
  globalParRcl(msg->master.tune);
  for(uint32_t i = 0; i < msg->scale.size(); i++)
  {
    globalParRcl(msg->scale[i]);
  }
  // local updates: unison, mono - updating va
  localPolyRcl(0, true, msg->unison, msg->mono);
  // local updates: unmodulateables
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: local unmodulateables/mc_times:");
  }
  for(uint32_t i = 0; i < msg->unmodulateables.size(); i++)
  {
    localParRcl(0, msg->unmodulateables[i]);
  }
  // local updates: modulateables
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: local modulateables:");
  }
  for(uint32_t i = 0; i < msg->modulateables.size(); i++)
  {
    localParRcl(0, msg->modulateables[i]);
  }
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: start transitions:");
  }
  // start transitions: global unmodulateables
  for(uint32_t i = 0; i < m_params.m_global.m_direct_count; i++)
  {
    auto param = m_params.get_global_direct(i);
    if(i == static_cast<uint32_t>(C15::Parameters::Global_Unmodulateables::Scale_Base_Key))
    {
      m_global.start_base_key(m_transition_time.m_dx.m_dx_slow, param->m_scaled);
    }
    else
    {
      globalTransition(param, m_transition_time.m_dx);
    }
  }
  // start transitions: global modulateables
  for(uint32_t i = 0; i < m_params.m_global.m_target_count; i++)
  {
    auto param = m_params.get_global_target(i);
    globalTransition(param, m_transition_time.m_dx);
  }
  // start transitions: local unmodulateables
  for(uint32_t i = 0; i < m_params.m_layer[0].m_direct_count; i++)
  {
    auto param = m_params.get_local_direct(0, i);
    for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
    {
      localTransition(layerId, param, m_transition_time.m_dx);
    }
  }
  // start transitions: local modulateables
  for(uint32_t i = 0; i < m_params.m_layer[0].m_target_count; i++)
  {
    auto param = m_params.get_local_target(0, i);
    for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
    {
      localTransition(layerId, param, m_transition_time.m_dx);
    }
  }
  // logging levels after recall for debugging switching dual modes
  if(LOG_RECALL_LEVELS)
  {
    debugLevels();
  }
}

void dsp_host_dual::recallSplit(const nltools::msg::SplitPresetMessage& _msg)
{
  auto oldLayerMode = std::exchange(m_layer_mode, LayerMode::Split);
  auto layerChanged = oldLayerMode != m_layer_mode;

  if(LOG_RECALL)
  {
    nltools::Log::info("recallSplit(@", m_clock.m_index, ")");
  }

  auto msg = &_msg;
  for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
  {
    const C15::Properties::LayerId layer = static_cast<C15::Properties::LayerId>(layerId);

    // update unison and mono groups
    auto polyChanged = evalPolyChg(layer, msg->unison[layerId].unisonVoices, msg->mono[layerId].monoEnable);

    // reset detection
    if(layerChanged || polyChanged)
    {
      if(LOG_RESET)
      {
        nltools::Log::info("recall split voice reset(layerId:", layerId, ")");
      }
      m_alloc.setUnison(layerId, m_params.get_local_unison_voices(layer)->m_position, oldLayerMode, m_layer_mode);
      m_alloc.setMonoEnable(layerId, m_params.get_local_mono_enable(layer)->m_position, m_layer_mode);
      const uint32_t uVoice = m_alloc.m_unison - 1;
      m_poly[layerId].resetEnvelopes();
      m_poly[layerId].m_uVoice = uVoice;
      m_poly[layerId].m_key_active = 0;
    }
    // reset macro assignments
    m_params.m_layer[layerId].m_assignment.reset();
    // reset voice fade
    m_poly[layerId].resetVoiceFade();
  }
  m_params.m_global.m_assignment.reset();
  // global updates: hw sources
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: hw sources:");
  }
  for(uint32_t i = 0; i < msg->hwsources.size(); i++)
  {
    globalParRcl(msg->hwsources[i]);
  }
  // global updates: hw amounts
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: hw amounts:");
  }
  for(uint32_t i = 0; i < msg->hwamounts.size(); i++)
  {
    globalParRcl(msg->hwamounts[i]);
  }
  // global updates: macros
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: macros:");
  }

  for(uint32_t i = 0; i < msg->macros.size(); i++)
  {
    globalParRcl(msg->macros[i]);
  }
  for(uint32_t i = 0; i < msg->macrotimes.size(); i++)
  {
    globalTimeRcl(msg->macrotimes[i]);
  }
  // global updates: parameters
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: global params (modulateables):");
  }
  globalParRcl(msg->master.volume);
  globalParRcl(msg->master.tune);
  globalParRcl(msg->splitpoint);
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: global params (unmodulateables):");
  }
  for(uint32_t i = 0; i < msg->scale.size(); i++)
  {
    globalParRcl(msg->scale[i]);
  }
  // local updates (each layer)
  for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
  {
    // local updates: unison, mono - updating va
    localPolyRcl(layerId, true, msg->unison[layerId], msg->mono[layerId]);
    // local updates: unmodulateables
    if(LOG_RECALL)
    {
      nltools::Log::info("recall: local unmodulateables/mc_times:");
    }
    for(uint32_t i = 0; i < msg->unmodulateables[layerId].size(); i++)
    {
      localParRcl(layerId, msg->unmodulateables[layerId][i]);
    }
    // local updates: modulateables
    if(LOG_RECALL)
    {
      nltools::Log::info("recall: local modulateables:");
    }
    for(uint32_t i = 0; i < msg->modulateables[layerId].size(); i++)
    {
      localParRcl(layerId, msg->modulateables[layerId][i]);
    }
  }
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: start transitions:");
  }
  // start transitions: global unmodulateables
  for(uint32_t i = 0; i < m_params.m_global.m_direct_count; i++)
  {
    auto param = m_params.get_global_direct(i);
    if(i == static_cast<uint32_t>(C15::Parameters::Global_Unmodulateables::Scale_Base_Key))
    {
      m_global.start_base_key(m_transition_time.m_dx.m_dx_slow, param->m_scaled);
    }
    else
    {
      globalTransition(param, m_transition_time.m_dx);
    }
  }
  // start transitions: global modulateables
  for(uint32_t i = 0; i < m_params.m_global.m_target_count; i++)
  {
    auto param = m_params.get_global_target(i);
    globalTransition(param, m_transition_time.m_dx);
  }
  for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
  {
    // start transitions: local unmodulateables
    for(uint32_t i = 0; i < m_params.m_layer[0].m_direct_count; i++)
    {
      auto param = m_params.get_local_direct(layerId, i);
      localTransition(layerId, param, m_transition_time.m_dx);
    }
    // start transitions: local modulateables
    for(uint32_t i = 0; i < m_params.m_layer[0].m_target_count; i++)
    {
      auto param = m_params.get_local_target(layerId, i);
      localTransition(layerId, param, m_transition_time.m_dx);
    }
  }
  // logging levels after recall for debugging switching dual modes
  if(LOG_RECALL_LEVELS)
  {
    debugLevels();
  }
}

void dsp_host_dual::recallLayer(const nltools::msg::LayerPresetMessage& _msg)
{
  auto oldLayerMode = std::exchange(m_layer_mode, LayerMode::Layer);
  auto layerChanged = oldLayerMode != m_layer_mode;

  if(LOG_RECALL)
  {
    nltools::Log::info("recallLayer(@", m_clock.m_index, ")");
  }
  auto msg = &_msg;
  // update unison and mono groups
  auto polyChanged = evalPolyChg(C15::Properties::LayerId::I, msg->unison.unisonVoices, msg->mono.monoEnable);
  // reset detection
  if(layerChanged || polyChanged)
  {
    if(LOG_RESET)
    {
      nltools::Log::info("recall layer voice reset");
    }
    m_alloc.setUnison(0, m_params.get_local_unison_voices(C15::Properties::LayerId::I)->m_position, oldLayerMode,
                      m_layer_mode);
    m_alloc.setMonoEnable(0, m_params.get_local_mono_enable(C15::Properties::LayerId::I)->m_position, m_layer_mode);
    const uint32_t uVoice = m_alloc.m_unison - 1;
    for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
    {
      m_poly[layerId].resetEnvelopes();
      m_poly[layerId].m_uVoice = uVoice;
      m_poly[layerId].m_key_active = 0;
    }
  }
  // reset macro assignments
  m_params.m_global.m_assignment.reset();
  for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
  {
    m_params.m_layer[layerId].m_assignment.reset();
  }
  // global updates: hw sources
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: hw sources:");
  }
  for(uint32_t i = 0; i < msg->hwsources.size(); i++)
  {
    globalParRcl(msg->hwsources[i]);
  }
  // global updates: hw amounts
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: hw amounts:");
  }
  for(uint32_t i = 0; i < msg->hwamounts.size(); i++)
  {
    globalParRcl(msg->hwamounts[i]);
  }
  // global updates: macros
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: macros:");
  }

  for(uint32_t i = 0; i < msg->macros.size(); i++)
  {
    globalParRcl(msg->macros[i]);
  }
  for(uint32_t i = 0; i < msg->macrotimes.size(); i++)
  {
    globalTimeRcl(msg->macrotimes[i]);
  }
  // global updates: parameters
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: global params (modulateables):");
  }
  globalParRcl(msg->master.volume);
  globalParRcl(msg->master.tune);
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: global params (unmodulateables):");
  }
  for(uint32_t i = 0; i < msg->scale.size(); i++)
  {
    globalParRcl(msg->scale[i]);
  }
  // local updates: unison, mono - updating va
  localPolyRcl(0, true, msg->unison, msg->mono);
  // transfer unison detune, phase, pan and mono glide to other part for proper smoother values - ignoring va
  localPolyRcl(1, false, msg->unison, msg->mono);
  // local updates (each layer)
  for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
  {
    // local updates: unmodulateables
    if(LOG_RECALL)
    {
      nltools::Log::info("recall: local unmodulateables/mc_times:");
    }
    for(uint32_t i = 0; i < msg->unmodulateables[layerId].size(); i++)
    {
      localParRcl(layerId, msg->unmodulateables[layerId][i]);
    }
    evalVoiceFadeChg(layerId);
    // local updates: modulateables
    if(LOG_RECALL)
    {
      nltools::Log::info("recall: local modulateables:");
    }
    for(uint32_t i = 0; i < msg->modulateables[layerId].size(); i++)
    {
      localParRcl(layerId, msg->modulateables[layerId][i]);
    }
  }
  if(LOG_RECALL)
  {
    nltools::Log::info("recall: start transitions:");
  }
  // start transitions: global unmodulateables
  for(uint32_t i = 0; i < m_params.m_global.m_direct_count; i++)
  {
    auto param = m_params.get_global_direct(i);
    if(i == static_cast<uint32_t>(C15::Parameters::Global_Unmodulateables::Scale_Base_Key))
    {
      m_global.start_base_key(m_transition_time.m_dx.m_dx_slow, param->m_scaled);
    }
    else
    {
      globalTransition(param, m_transition_time.m_dx);
    }
  }
  // start transitions: global modulateables
  for(uint32_t i = 0; i < m_params.m_global.m_target_count; i++)
  {
    auto param = m_params.get_global_target(i);
    globalTransition(param, m_transition_time.m_dx);
  }
  for(uint32_t layerId = 0; layerId < m_params.m_layer_count; layerId++)
  {
    // start transitions: local unmodulateables
    for(uint32_t i = 0; i < m_params.m_layer[0].m_direct_count; i++)
    {
      auto param = m_params.get_local_direct(layerId, i);
      localTransition(layerId, param, m_transition_time.m_dx);
    }
    // start transitions: local modulateables
    for(uint32_t i = 0; i < m_params.m_layer[0].m_target_count; i++)
    {
      auto param = m_params.get_local_target(layerId, i);
      localTransition(layerId, param, m_transition_time.m_dx);
    }
  }
  // logging levels after recall for debugging switching dual modes
  if(LOG_RECALL_LEVELS)
  {
    debugLevels();
  }
}

void dsp_host_dual::globalParRcl(const nltools::msg::ParameterGroups::HardwareSourceParameter& _param)
{
  auto element = getParameter(_param.id);
  if(element.m_param.m_type == C15::Descriptors::ParameterType::Hardware_Source)
  {
    auto param = m_params.get_hw_src(element.m_param.m_index);
    param->update_behavior(getBehavior(_param.returnMode));
    param->update_position(static_cast<float>(_param.controlPosition));
  }
  else if(LOG_FAIL)
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "failed to recall HW Source(id:", _param.id, ")");
  }
}
void dsp_host_dual::globalParRcl(const nltools::msg::ParameterGroups::HardwareAmountParameter& _param)
{
  auto element = getParameter(_param.id);
  if(element.m_param.m_type == C15::Descriptors::ParameterType::Hardware_Amount)
  {
    auto param = m_params.get_hw_amt(element.m_param.m_index);
    param->update_position(static_cast<float>(_param.controlPosition));
  }
  else if(LOG_FAIL)
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "failed to recall HW Amount(id:", _param.id, ")");
  }
}

void dsp_host_dual::globalParRcl(const nltools::msg::ParameterGroups::MacroParameter& _param)
{
  auto element = getParameter(_param.id);
  if(element.m_param.m_type == C15::Descriptors::ParameterType::Macro_Control)
  {
    auto param = m_params.get_macro(element.m_param.m_index);
    param->update_position(static_cast<float>(_param.controlPosition));
  }
  else if(LOG_FAIL)
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "failed to recall Macro(id:", _param.id, ")");
  }
}

void dsp_host_dual::globalParRcl(const nltools::msg::ParameterGroups::ModulateableParameter& _param)
{
  auto element = getParameter(_param.id);
  if(element.m_param.m_type == C15::Descriptors::ParameterType::Global_Modulateable)
  {
    if(LOG_RECALL_DETAILS)
    {
      nltools::Log::info("recall(id:", _param.id, ", label:", element.m_pg.m_group_label_short,
                         element.m_pg.m_param_label_short, ", value:", _param.controlPosition,
                         ", initial:", element.m_initial, ")");
    }
    auto param = m_params.get_global_target(element.m_param.m_index);
    param->update_source(getMacro(_param.mc));
    param->update_amount(static_cast<float>(_param.modulationAmount));
    param->update_position(param->depolarize(static_cast<float>(_param.controlPosition)));
    param->m_scaled = scale(param->m_scaling, param->polarize(param->m_position));
    const uint32_t macroId = getMacroId(_param.mc);
    m_params.m_global.m_assignment.reassign(element.m_param.m_index, macroId);
    param->update_modulation_aspects(m_params.get_macro(macroId)->m_position);
  }
  else if(LOG_FAIL)
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "failed to recall Global Modulateable(id:", _param.id, ")");
  }
}

void dsp_host_dual::globalParRcl(const nltools::msg::ParameterGroups::SplitPoint& _param)
{
  auto element = getParameter(_param.id);
  if(element.m_param.m_index == static_cast<uint32_t>(C15::Parameters::Global_Modulateables::Split_Split_Point))
  {
    if(LOG_RECALL_DETAILS)
    {
      nltools::Log::info(__PRETTY_FUNCTION__, "recall(id:", _param.id, ", label:", element.m_pg.m_group_label_short,
                         element.m_pg.m_param_label_short, ", value:", _param.controlPosition,
                         ", initial:", element.m_initial, ")");
    }
    auto param = m_params.get_global_target(element.m_param.m_index);
    param->update_source(getMacro(_param.mc));
    param->update_amount(static_cast<float>(_param.modulationAmount));
    param->update_position(param->depolarize(static_cast<float>(_param.controlPosition)));
    param->m_scaled = scale(param->m_scaling, param->polarize(param->m_position));
    const uint32_t macroId = getMacroId(_param.mc);
    m_params.m_global.m_assignment.reassign(element.m_param.m_index, macroId);
    param->update_modulation_aspects(m_params.get_macro(macroId)->m_position);
    m_alloc.setSplitPoint(static_cast<uint32_t>(param->m_scaled));
  }
  else if(LOG_FAIL)
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "failed to recall Global Modulateable(id:", _param.id, ")");
  }
}

void dsp_host_dual::globalParRcl(const nltools::msg::ParameterGroups::UnmodulateableParameter& _param)
{
  auto element = getParameter(_param.id);
  if(element.m_param.m_type == C15::Descriptors::ParameterType::Global_Unmodulateable)
  {
    if(LOG_RECALL_DETAILS)
    {
      nltools::Log::info(__PRETTY_FUNCTION__, "recall(id:", _param.id, ", label:", element.m_pg.m_group_label_short,
                         element.m_pg.m_param_label_short, ", value:", _param.controlPosition,
                         ", initial:", element.m_initial, ")");
    }
    auto param = m_params.get_global_direct(element.m_param.m_index);
    param->update_position(static_cast<float>(_param.controlPosition));
    param->m_scaled = scale(param->m_scaling, param->m_position);
  }
  else if(LOG_FAIL)
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "failed to recall Global Unmodulateable(id:", _param.id, ")");
  }
}

void dsp_host_dual::globalParRcl(const nltools::msg::ParameterGroups::GlobalParameter& _param)
{
  auto element = getParameter(_param.id);
  if(element.m_param.m_type == C15::Descriptors::ParameterType::Global_Unmodulateable)
  {
    if(LOG_RECALL_DETAILS)
    {
      nltools::Log::info(__PRETTY_FUNCTION__, "recall(id:", _param.id, ", label:", element.m_pg.m_group_label_short,
                         element.m_pg.m_param_label_short, ", value:", _param.controlPosition,
                         ", initial:", element.m_initial, ")");
    }
    auto param = m_params.get_global_direct(element.m_param.m_index);
    param->update_position(static_cast<float>(_param.controlPosition));
    param->m_scaled = scale(param->m_scaling, param->m_position);
  }
  else if(LOG_FAIL)
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "failed to recall GlobalParameter(id:", _param.id, ")");
  }
}

void dsp_host_dual::globalTimeRcl(const nltools::msg::ParameterGroups::UnmodulateableParameter& _param)
{
  auto element = getParameter(_param.id);
  if(element.m_param.m_type == C15::Descriptors::ParameterType::Macro_Time)
  {
    auto param = m_params.get_macro_time(element.m_param.m_index);
    param->update_position(static_cast<float>(_param.controlPosition));
    updateTime(&param->m_dx, scale(param->m_scaling, param->m_position));
  }
  else if(LOG_FAIL)
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "failed to recall Macro Time(id:", _param.id, ")");
  }
}
void dsp_host_dual::localParRcl(const uint32_t _layerId,
                                const nltools::msg::ParameterGroups::ModulateableParameter& _param)
{
  auto element = getParameter(_param.id);
  if(element.m_param.m_type == C15::Descriptors::ParameterType::Local_Modulateable)
  {
    if(LOG_RECALL_DETAILS)
    {
      nltools::Log::info(__PRETTY_FUNCTION__, "recall(layer:", _layerId, ", id:", _param.id,
                         ", label:", element.m_pg.m_group_label_short, element.m_pg.m_param_label_short,
                         ", value:", _param.controlPosition, ", initial:", element.m_initial, ")");
    }
    auto param = m_params.get_local_target(_layerId, element.m_param.m_index);
    param->update_source(getMacro(_param.mc));
    param->update_amount(static_cast<float>(_param.modulationAmount));
    param->update_position(param->depolarize(static_cast<float>(_param.controlPosition)));
    param->m_scaled = scale(param->m_scaling, param->polarize(param->m_position));
    const uint32_t macroId = getMacroId(_param.mc);
    m_params.m_layer[_layerId].m_assignment.reassign(element.m_param.m_index, macroId);
    param->update_modulation_aspects(m_params.get_macro(macroId)->m_position);
  }
  else if(LOG_FAIL)
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "failed to recall Local Target(id:", _param.id, ")");
  }
}

void dsp_host_dual::localParRcl(const uint32_t _layerId,
                                const nltools::msg::ParameterGroups::UnmodulateableParameter& _param)
{
  auto element = getParameter(_param.id);
  if(element.m_param.m_type == C15::Descriptors::ParameterType::Local_Unmodulateable)
  {
    if(LOG_RECALL_DETAILS)
    {
      nltools::Log::info(__PRETTY_FUNCTION__, "recall(layer:", _layerId, ", id:", _param.id,
                         ", label:", element.m_pg.m_group_label_short, element.m_pg.m_param_label_short,
                         ", value:", _param.controlPosition, ", initial:", element.m_initial, ")");
    }
    auto param = m_params.get_local_direct(_layerId, element.m_param.m_index);
    param->update_position(static_cast<float>(_param.controlPosition));
    param->m_scaled = scale(param->m_scaling, param->m_position);
  }
  else if(LOG_FAIL)
  {
    nltools::Log::warning(__PRETTY_FUNCTION__, "failed to recall Local Direct(id:", _param.id, ")");
  }
}

void dsp_host_dual::localPolyRcl(const uint32_t _layerId, const bool _va_update,
                                 const nltools::msg::ParameterGroups::UnisonGroup& _unison,
                                 const nltools::msg::ParameterGroups::MonoGroup& _mono)
{
  localParRcl(_layerId, _unison.detune);
  localParRcl(_layerId, _unison.pan);
  localParRcl(_layerId, _unison.phase);
  localParRcl(_layerId, _mono.glide);
  // should the voice allocation be updated? (depends on layer mode)
  if(_va_update)
  {
    localParRcl(_layerId, _mono.priority);
    localParRcl(_layerId, _mono.legato);
    const C15::Properties::LayerId layerId = static_cast<C15::Properties::LayerId>(_layerId);
    m_alloc.setMonoPriority(_layerId, m_params.get_local_mono_priority(layerId)->m_scaled, m_layer_mode);
    m_alloc.setMonoLegato(_layerId, m_params.get_local_mono_legato(layerId)->m_scaled, m_layer_mode);
  }
}

void dsp_host_dual::debugLevels()
{
  nltools::Log::info(
      "MasterLevel:",
      m_params.get_global_direct(static_cast<uint32_t>(C15::Parameters::Global_Modulateables::Master_Volume))
          ->m_scaled);
  nltools::Log::info(
      "VoiceGroupLevel[I]:",
      m_params.get_local_target(0, static_cast<uint32_t>(C15::Parameters::Local_Modulateables::Voice_Grp_Volume))
          ->m_scaled);
  nltools::Log::info(
      "VoiceGroupLevel[II]:",
      m_params.get_local_target(0, static_cast<uint32_t>(C15::Parameters::Local_Modulateables::Voice_Grp_Volume))
          ->m_scaled);
}

#if __POTENTIAL_IMPROVEMENT_NUMERIC_TESTS__
void dsp_host_dual::PotentialImprovements_RunNumericTests()
{
  // startup: provide test data
  nltools::Log::info(__PRETTY_FUNCTION__, "starting tests (proposal_enabled:", __POTENTIAL_IMPROVEMENT_PROPOSAL__, ")");
  const float TestGroup_Pattern_data[12]
      = { -1.0f, -0.99f, -0.75f, -0.5f, -0.3f, -0.0f, 0.0f, 0.3f, 0.5f, 0.75f, 0.99f, 1.0f };
  const PolyValue TestGroup_Pattern{ TestGroup_Pattern_data };
  const size_t TestGroups = 4;
  const char* RunInfo[TestGroups] = { "big", "unclamped", "clamped", "small" };
  const PolyValue TestGroup[TestGroups]
      = { 500.5f * TestGroup_Pattern, 2.25f * TestGroup_Pattern, 1.0f * TestGroup_Pattern, 0.001f * TestGroup_Pattern };
  const float Threshold = 1.e-18f;

  // consider enabled implementations individually

  nltools::Log::info("  POTENTIAL_IMPROVEMENT_COMB_REDUCE_VOICE_LOOP_1:");
  for(int i = 0; i < TestGroups; i++)
  {
    uint16_t Profile = 0;
    auto tmp1 = TestGroup[i];
    // potential improvement - parallel implementation
    auto tmp2 = std::abs(tmp1);
    const PolyInt tmp2_sign_condition((tmp1 > 0.0f)),  // contains -1 (true) or 0 (false)
        tmp2_sat_condition((tmp2 > 0.501187f));        // contains -1 (true) or 0 (false)
    tmp2 -= 0.501187f;
    auto tmp3 = tmp2 * 0.2512f;
    tmp2 = std::min(tmp2, 2.98815f);
    tmp2 *= 0.7488f * (1.0f - (tmp2 * 0.167328f));
    tmp2 += tmp3 + 0.501187f;
    tmp2 *= static_cast<PolyValue>((-2 * tmp2_sign_condition) - 1);  // restore sign
    tmp3 = tmp2 - tmp1;                                              // detect difference to unsaturated signal
    tmp1 -= static_cast<PolyValue>(tmp2_sat_condition) * tmp3;       // restore signal
    nltools::Log::info("    [ voiceId : improved, current, difference = abs(parallel - scalar) ] - run", i + 1, "of",
                       TestGroups, "(", RunInfo[i], "numbers )");
    // scalar implementation within voice loop
    auto tmp4 = TestGroup[i];
    for(size_t v = 0; v < 12; v++)
    {
      // current implementation
      if(std::abs(tmp4[v]) > 0.501187f)
      {
        if(tmp4[v] > 0.f)
        {
          tmp4[v] -= 0.501187f;
          auto tmp5 = tmp4[v];
          tmp4[v] = std::min(tmp4[v], 2.98815f);
          tmp4[v] *= (1.0f - tmp4[v] * 0.167328f);
          tmp4[v] *= 0.7488f;
          tmp5 *= 0.2512f;
          tmp4[v] += (tmp5 + 0.501187f);
        }
        else
        {
          tmp4[v] += 0.501187f;
          auto tmp5 = tmp4[v];
          tmp4[v] = std::max(tmp4[v], -2.98815f);
          tmp4[v] *= (1.0f - std::abs(tmp4[v]) * 0.167328f);
          tmp4[v] *= 0.7488f;
          tmp5 *= 0.2512f;
          tmp4[v] += (tmp5 - 0.501187f);
        }
      }
      // value comparison
      const auto tmpAbs = std::abs(tmp1[v] - tmp4[v]);
      if(tmpAbs > Threshold)
      {
        nltools::Log::info("      [", v, ":", tmp1[v], ",", tmp4[v], ",", tmpAbs, "]");
        Profile |= 1;
      }
      Profile <<= 1;
    }
    nltools::Log::info("    Profile:", Profile);
  }

  nltools::Log::info("  POTENTIAL_IMPROVEMENT_PARALLEL_DATA_STD_ABS:");
  for(int i = 0; i < TestGroups; i++)
  {
    uint16_t Profile = 0;
    // potential improvement - parallel implementation
    auto tmp1 = std::abs(TestGroup[i]);
    nltools::Log::info("    [ voiceId : improved, current, difference = abs(parallel - scalar) ] - run", i + 1, "of",
                       TestGroups, "(", RunInfo[i], "numbers )");
    // scalar implementation within voice loop
    auto tmp2 = TestGroup[i];
    for(size_t v = 0; v < 12; v++)
    {
      // scalar implementation
      auto tmp3 = abs(tmp2[v]);
      // value comparison
      const auto tmpAbs = std::abs(tmp1[v] - tmp3);
      if(tmpAbs > Threshold)
      {
        nltools::Log::info("      [", v, ":", tmp1[v], ",", tmp3, ",", tmpAbs, "]");
        Profile |= 1;
      }
      Profile <<= 1;
    }
    nltools::Log::info("    Profile:", Profile);
  }

  nltools::Log::info("  POTENTIAL_IMPROVEMENT_PARALLEL_DATA_STD_MINMAX_FLOAT:");
  for(int i = 0; i < TestGroups; i++)
  {
    uint16_t Profile = 0;
    const float tmpMin = -1.0f, tmpMax = 1.0f;
    // potential improvement - parallel implementation:
    // - min(parallel,scalar), max(parallel,scalar), clamp(parallel,scalar,scalar), clamp(parallel,scalar,parallel)
    auto tmp1 = std::min(TestGroup[i], tmpMin), tmp2 = std::max(TestGroup[i], tmpMax),
         tmp3 = std::clamp(TestGroup[i], tmpMin, tmpMax), tmp4 = std::clamp(TestGroup[i], tmpMin, TestGroup[2]);
    nltools::Log::info("    [ voiceId : improved, current, difference = abs(parallel - scalar) ] - run", i + 1, "of",
                       TestGroups, "(", RunInfo[i], "numbers )");
    // scalar implementation within voice loop
    for(size_t v = 0; v < 12; v++)
    {
      // scalar implementation
      auto tmp5 = std::min(TestGroup[i][v], tmpMin), tmp6 = std::max(TestGroup[i][v], tmpMax),
           tmp7 = std::clamp(TestGroup[i][v], tmpMin, tmpMax),
           tmp8 = std::clamp(TestGroup[i][v], tmpMin, TestGroup[2][v]);
      // value comparison
      const auto tmpAbs1 = std::abs(tmp1[v] - tmp5), tmpAbs2 = std::abs(tmp2[v] - tmp6),
                 tmpAbs3 = std::abs(tmp3[v] - tmp7), tmpAbs4 = std::abs(tmp4[v] - tmp8),
                 tmpAbs = tmpAbs1 + tmpAbs2 + tmpAbs3 + tmpAbs4;
      if(tmpAbs > Threshold)
      {
        nltools::Log::info("      [", v, ":", tmp1[v], ",", tmp5, ",", tmpAbs1, "] ( std::min(parallel, scalar) )");
        nltools::Log::info("      [", v, ":", tmp2[v], ",", tmp6, ",", tmpAbs2, "] ( std::max(parallel, scalar) )");
        nltools::Log::info("      [", v, ":", tmp3[v], ",", tmp7, ",", tmpAbs3,
                           "] ( std::clamp(parallel, scalar, scalar) )");
        nltools::Log::info("      [", v, ":", tmp4[v], ",", tmp8, ",", tmpAbs4,
                           "] ( std::clamp(parallel, scalar, parallel) )");
        Profile |= 1;
      }
      Profile <<= 1;
    }
    nltools::Log::info("    Profile:", Profile);
  }

  nltools::Log::info("  POTENTIAL_IMPROVEMENT_PARALLEL_DATA_STD_ROUND:");
  for(int i = 0; i < TestGroups; i++)
  {
    uint16_t Profile = 0;
    // potential improvement - parallel implementation
    auto tmp1 = static_cast<PolyValue>(std::round<int32_t>(TestGroup[i]));
    nltools::Log::info("    [ voiceId : improved, current, difference = abs(parallel - scalar) ] - run", i + 1, "of",
                       TestGroups, "(", RunInfo[i], "numbers )");
    // scalar implementation within voice loop
    for(size_t v = 0; v < 12; v++)
    {
      // scalar implementation
      auto tmp2 = std::round(TestGroup[i][v]);
      // value comparison
      const auto tmpAbs = std::abs(tmp1[v] - tmp2);
      if(tmpAbs > Threshold)
      {
        nltools::Log::info("      [", v, ":", tmp1[v], ",", tmp2, ",", tmpAbs, "]");
        Profile |= 1;
      }
      Profile <<= 1;
    }
    nltools::Log::info("    Profile:", Profile);
  }

  nltools::Log::info("  POTENTIAL_IMPROVEMENT_PARALLEL_DATA_KEEP_FRACTIONAL:");
  for(int i = 0; i < TestGroups; i++)
  {
    uint16_t Profile = 0;
    // potential improvement - parallel implementation
    auto tmp1 = keepFractional(TestGroup[i]);
    nltools::Log::info("    [ voiceId : improved, current, difference = abs(parallel - scalar) ] - run", i + 1, "of",
                       TestGroups, "(", RunInfo[i], "numbers )");
    // scalar implementation within voice loop
    for(size_t v = 0; v < 12; v++)
    {
      // scalar implementation
      auto tmp2 = TestGroup[i][v] - NlToolbox::Conversion::float2int(TestGroup[i][v]);
      // value comparison
      const auto tmpAbs = std::abs(tmp1[v] - tmp2);
      if(tmpAbs > Threshold)
      {
        nltools::Log::info("      [", v, ":", tmp1[v], ",", tmp2, ",", tmpAbs, "]");
        Profile |= 1;
      }
      Profile <<= 1;
    }
    nltools::Log::info("    Profile:", Profile);
  }

  nltools::Log::info("  POTENTIAL_IMPROVEMENT_PARALLEL_DATA_SINP3_WRAP:");
  for(int i = 0; i < TestGroups; i++)
  {
    uint16_t Profile = 0;
    // potential improvement - parallel implementation
    auto tmp1 = sinP3_wrap(TestGroup[i]);
    nltools::Log::info("    [ voiceId : improved, current, difference = abs(parallel - scalar) ] - run", i + 1, "of",
                       TestGroups, "(", RunInfo[i], "numbers )");
    // scalar implementation within voice loop
    for(size_t v = 0; v < 12; v++)
    {
      // scalar implementation
      auto tmp2 = NlToolbox::Math::sinP3_wrap(TestGroup[i][v]);
      // value comparison
      const auto tmpAbs = std::abs(tmp1[v] - tmp2);
      if(tmpAbs > Threshold)
      {
        nltools::Log::info("      [", v, ":", tmp1[v], ",", tmp2, ",", tmpAbs, "]");
        Profile |= 1;
      }
      Profile <<= 1;
    }
    nltools::Log::info("    Profile:", Profile);
  }

  nltools::Log::info("  POTENTIAL_IMPROVEMENT_PARALLEL_DATA_SINP3_NOWRAP:");
  for(int i = 0; i < TestGroups; i++)
  {
    uint16_t Profile = 0;
    // potential improvement - parallel implementation
    auto tmp1 = sinP3_noWrap(TestGroup[i]);
    nltools::Log::info("    [ voiceId : improved, current, difference = abs(parallel - scalar) ] - run", i + 1, "of",
                       TestGroups, "(", RunInfo[i], "numbers )");
    // scalar implementation within voice loop
    for(size_t v = 0; v < 12; v++)
    {
      // scalar implementation
      auto tmp2 = NlToolbox::Math::sinP3_noWrap(TestGroup[i][v]);
      // value comparison
      const auto tmpAbs = std::abs(tmp1[v] - tmp2);
      if(tmpAbs > Threshold)
      {
        nltools::Log::info("      [", v, ":", tmp1[v], ",", tmp2, ",", tmpAbs, "]");
        Profile |= 1;
      }
      Profile <<= 1;
    }
    nltools::Log::info("    Profile:", Profile);
  }

  nltools::Log::info("  POTENTIAL_IMPROVEMENT_PARALLEL_DATA_THREE_RANGES:");
  for(int i = 0; i < TestGroups; i++)
  {
    uint16_t Profile = 0;
    // potential improvement - parallel implementation
    const float fold = 0.5f;
    auto tmp1 = threeRanges(TestGroup[i], TestGroup[2], fold);
    nltools::Log::info("    [ voiceId : improved, current, difference = abs(parallel - scalar) ] - run", i + 1, "of",
                       TestGroups, "(", RunInfo[i], "numbers )");
    // scalar implementation within voice loop
    for(size_t v = 0; v < 12; v++)
    {
      auto tmp2 = TestGroup[i][v];
      // scalar implementation
      if(TestGroup[2][v] < -0.25f)
      {
        tmp2 = (tmp2 + 1.f) * fold - 1.f;
      }
      else if(TestGroup[2][v] > -0.25f)
      {
        tmp2 = (tmp2 - 1.f) * fold + 1.f;
      }
      // value comparison
      const auto tmpAbs = std::abs(tmp1[v] - tmp2);
      if(tmpAbs > Threshold)
      {
        nltools::Log::info("      [", v, ":", tmp1[v], ",", tmp2, ",", tmpAbs, "]");
        Profile |= 1;
      }
      Profile <<= 1;
    }
    nltools::Log::info("    Profile:", Profile);
  }

  nltools::Log::info(__PRETTY_FUNCTION__, "finished tests");
}
#endif
