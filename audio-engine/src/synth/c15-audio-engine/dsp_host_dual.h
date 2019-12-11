#pragma once

/******************************************************************************/
/** @file       dsp_host_dual.h
    @date
    @version    1.7-0
    @author     M. Seeber
    @brief      new main engine container
    @todo
*******************************************************************************/

#include "parameter-db/generated/c15_config.h"
#include "parameter-db/generated/parameter_list.h"
#include <nltools/messaging/Message.h>

#include "parameter_handle.h"
#include "pe_exponentiator.h"
#include "voice_allocation.h"

#include "ae_global_section.h"
#include "ae_poly_section.h"
#include "ae_mono_section.h"
#include "ae_fadepoint.h"

// basic logging switches
inline constexpr bool LOG_MISSING = true;
inline constexpr bool LOG_FAIL = true;
inline constexpr bool LOG_INIT = true;
inline constexpr bool LOG_MIDI = false;
inline constexpr bool LOG_MIDI_DETAIL = true;
inline constexpr bool LOG_DISPATCH = false;
inline constexpr bool LOG_EDITS = true;
inline constexpr bool LOG_TIMES = true;
inline constexpr bool LOG_SETTINGS = true;
inline constexpr bool LOG_RECALL = true;
inline constexpr bool LOG_RECALL_COMPARE_INITIAL = false;
inline constexpr bool LOG_RECALL_LEVELS = true;
inline constexpr bool LOG_KEYS = true;
inline constexpr bool LOG_KEYS_POLY = true;
inline constexpr bool LOG_TRANSITIONS = false;
inline constexpr bool LOG_RESET = true;
inline constexpr bool LOG_HW = true;
// modifiers/settings
inline constexpr bool RECALL_INITIAL_ONINIT = true;
inline constexpr bool RECALL_TRANSITION_ONCHANGE = false;

class dsp_host_dual
{
 public:
  // public members
  float m_mainOut_R, m_mainOut_L;
  // constructor
  dsp_host_dual();
  // public methods
  void init(const uint32_t _samplerate, const uint32_t _polyphony);
  // handles for inconvenient stuff
  C15::ParameterDescriptor getParameter(const int _id);
  // event bindings: debug
  void logStatus();
  // event bindings: LPC or MIDI Device (in Dev_PC mode)
  void onMidiMessage(const uint32_t _status, const uint32_t _data0, const uint32_t _data1);
  void onRawMidiMessage(const uint32_t _status, const uint32_t _data0, const uint32_t _data1);
  // event bindings: Preset Messages
  void onPresetMessage(const nltools::msg::SinglePresetMessage &_msg);
  void onPresetMessage(const nltools::msg::SplitPresetMessage &_msg);
  void onPresetMessage(const nltools::msg::LayerPresetMessage &_msg);
  // event bindings: Parameter Changed Messages
  void globalParChg(const uint32_t _id, const nltools::msg::HWSourceChangedMessage &_msg);
  void globalParChg(const uint32_t _id, const nltools::msg::HWAmountChangedMessage &_msg);
  void globalParChg(const uint32_t _id, const nltools::msg::MacroControlChangedMessage &_msg);
  void globalParChg(const uint32_t _id, const nltools::msg::ModulateableParameterChangedMessage &_msg);
  void globalParChg(const uint32_t _id, const nltools::msg::UnmodulateableParameterChangedMessage &_msg);
  void globalTimeChg(const uint32_t _id, const nltools::msg::UnmodulateableParameterChangedMessage &_msg);
  void localParChg(const uint32_t _id, const nltools::msg::ModulateableParameterChangedMessage &_msg);
  void localParChg(const uint32_t _id, const nltools::msg::UnmodulateableParameterChangedMessage &_msg);
  void localUnisonChg(const nltools::msg::UnmodulateableParameterChangedMessage &_msg);
  // evend bindings: Settings
  void onSettingEditTime(const float _position);
  void onSettingTransitionTime(const float _position);
  void onSettingNoteShift(const float _shift);
  void onSettingGlitchSuppr(const bool _enabled);
  void onSettingInitialSinglePreset();
  uint32_t onSettingToneToggle();
  // dsp-related
  void render();
  void reset();

 private:
  // preloadable preset buffers
  nltools::msg::SinglePresetMessage m_preloaded_single_data;
  nltools::msg::SplitPresetMessage m_preloaded_split_data;
  nltools::msg::LayerPresetMessage m_preloaded_layer_data;
  // parameters
  Engine::Param_Handle m_params;
  Time_Param m_edit_time, m_transition_time;
  Setting_Param m_reference;
  const C15::ParameterDescriptor m_invalid_param = { C15::None };
  // essential tools
  exponentiator m_convert;
  Engine::Handle::Clock_Handle m_clock;
  Engine::Handle::Time_Handle m_time;
  // layer handling
  C15::Properties::LayerMode m_layer_mode, m_preloaded_layer_mode;
  uint32_t m_layer_focus;  // probably obsolete
  // global dsp components
  GlobalSection m_global;
  VoiceAllocation<C15::Config::total_polyphony, C15::Config::local_polyphony, C15::Config::key_count> m_alloc;
  // dsp components
  ae_fade_table m_fade;
  ae_fader m_output_mute;
  PolySection m_poly[2];
  MonoSection m_mono[2];
  LayerSignalCollection m_z_layers[2];
  // helper values
  const float m_format_vel = 4095.0f / 127.0f, m_format_hw = 8000.0f / 127.0f, m_format_pb = 16000.0f / 16383.0f,
              m_norm_vel = 1.0f / 4095.0f, m_norm_hw = 1.0f / 8000.0f;
  uint32_t m_key_pos = 0, m_tone_state = 0;
  bool m_key_valid = false, m_layer_changed = false, m_glitch_suppression = false;
  // handles for inconvenient stuff
  C15::Properties::HW_Return_Behavior getBehavior(const ReturnMode _mode);
  C15::Properties::HW_Return_Behavior getBehavior(const RibbonReturnMode _mode);
  C15::Parameters::Macro_Controls getMacro(const MacroControls _mc);
  uint32_t getMacroId(const MacroControls _mc);
  C15::Properties::LayerId getLayer(const VoiceGroup _vg);
  uint32_t getLayerId(const VoiceGroup _vg);
  // key events
  void keyDown(const float _vel);
  void keyUp(const float _vel);
  float scale(const Scale_Aspect _scl, float _value);
  // inner event flow
  void updateHW(const uint32_t _id, const float _raw);
  void updateTime(Time_Aspect *_param, const float _ms);
  void hwModChain(HW_Src_Param *_src, const uint32_t _id, const float _inc);
  void globalModChain(Macro_Param *_mc);
  void localModChain(Macro_Param *_mc);
  void localModChain(const uint32_t _layer, Macro_Param *_mc);
  void globalTransition(const Target_Param *_param, const Time_Aspect _time);
  void globalTransition(const Direct_Param *_param, const Time_Aspect _time);
  void localTransition(const uint32_t _layer, const Direct_Param *_param, const Time_Aspect _time);
  void localTransition(const uint32_t _layer, const Target_Param *_param, const Time_Aspect _time);
  void evalFadePoint();
  Direct_Param *evalVoiceChg(const C15::Properties::LayerId _layerId,
                             const nltools::msg::ParameterGroups::UnmodulateableParameter &_unisonVoices);
  void recallSingle();
  void recallSplit();
  void recallLayer();
  void globalParRcl(const nltools::msg::ParameterGroups::HardwareSourceParameter &_param);
  void globalParRcl(const nltools::msg::ParameterGroups::HardwareAmountParameter &_param);
  void globalParRcl(const nltools::msg::ParameterGroups::MacroParameter &_param);
  void globalParRcl(const nltools::msg::ParameterGroups::ModulateableParameter &_param);
  void globalParRcl(const nltools::msg::ParameterGroups::UnmodulateableParameter &_param);
  void globalParRcl(const nltools::msg::ParameterGroups::GlobalParameter &_param);
  void globalTimeRcl(const nltools::msg::ParameterGroups::UnmodulateableParameter &_param);
  void localParRcl(const uint32_t _layerId, const nltools::msg::ParameterGroups::ModulateableParameter &_param);
  void localParRcl(const uint32_t _layerId, const nltools::msg::ParameterGroups::UnmodulateableParameter &_param);
  void debugLevels();
};
