#include <Application.h>
#include <device-settings/DebugLevel.h>
#include <parameters/Parameter.h>
#include <parameters/MacroControlParameter.h>
#include <presets/EditBuffer.h>
#include <presets/PresetManager.h>
#include <proxies/hwui/HWUI.h>
#include <proxies/hwui/panel-unit/PanelUnit.h>
#include <proxies/hwui/panel-unit/PanelUnitPresetMode.h>
#include <proxies/hwui/TwoStateLED.h>
#include <memory>
#include "PanelUnitPresetMode.h"
#include <device-settings/HighlightChangedParametersSetting.h>

PanelUnitPresetMode::PanelUnitPresetMode()
    : m_bruteForceLedThrottler(std::chrono::milliseconds(40))
{
  DebugLevel::gassy(__PRETTY_FUNCTION__);

  Application::get().getHWUI()->onModifiersChanged(
      sigc::hide(sigc::mem_fun(this, &PanelUnitPresetMode::bruteForceUpdateLeds)));

  Application::get().getPresetManager()->getEditBuffer()->onChange(
      mem_fun(this, &PanelUnitPresetMode::bruteForceUpdateLeds));

  Application::get().getHWUI()->onCurrentVoiceGroupChanged(
      sigc::hide(sigc::mem_fun(this, &PanelUnitPresetMode::bruteForceUpdateLeds)));
}

PanelUnitPresetMode::~PanelUnitPresetMode()
{
  Application::get().getSettings()->getSetting<ForceHighlightChangedParametersSetting>()->set(
      BooleanSetting::tEnum::BOOLEAN_SETTING_FALSE);
  DebugLevel::gassy(__PRETTY_FUNCTION__);
}

void PanelUnitPresetMode::bruteForceUpdateLeds()
{
  m_bruteForceLedThrottler.doTask([this]() {
    if(Application::get().getHWUI()->getPanelUnit().getUsageMode().get() != this)
      return;

    std::array<TwoStateLED::LedState, numLeds> states { TwoStateLED::OFF };

    if(Application::get().getHWUI()->getButtonModifiers()[SHIFT] == true)
      getMappings().forEachButton([&](Buttons buttonId, const std::list<int>& parameters) {
        Application::get().getSettings()->getSetting<ForceHighlightChangedParametersSetting>()->set(
            BooleanSetting::tEnum::BOOLEAN_SETTING_TRUE);
        letChangedButtonsBlink(buttonId, parameters, states);
      });
    else
      getMappings().forEachButton([&](Buttons buttonId, const std::list<int>& parameters) {
        Application::get().getSettings()->getSetting<ForceHighlightChangedParametersSetting>()->set(
            BooleanSetting::tEnum::BOOLEAN_SETTING_FALSE);
        setStateForButton(buttonId, parameters, states);
      });

    applyStateToLeds(states);
  });
}

void PanelUnitPresetMode::letChangedButtonsBlink(Buttons buttonId, const std::list<int>& parameters,
                                                 std::array<TwoStateLED::LedState, numLeds>& states)
{
  auto editBuffer = Application::get().getPresetManager()->getEditBuffer();
  auto vg = Application::get().getHWUI()->getCurrentVoiceGroup();
  auto currentParams = getMappings().findParameters(buttonId);
  auto ebParameters = editBuffer->getParametersSortedByNumber(vg);
  auto globalParameters = editBuffer->getParametersSortedByNumber(VoiceGroup::Global);

  bool anyChanged = false;
  for(const auto paramID : currentParams)
  {
    try
    {
      anyChanged |= ebParameters.at(paramID)->isChangedFromLoaded();
    }
    catch(...)
    {
      anyChanged |= globalParameters.at(paramID)->isChangedFromLoaded();
    }
  }
  states[(int) buttonId] = anyChanged ? TwoStateLED::BLINK : TwoStateLED::OFF;
}

void PanelUnitPresetMode::setStateForButton(Buttons buttonId, const std::list<int>& parameters,
                                            std::array<TwoStateLED::LedState, numLeds>& states)
{
  auto editBuffer = Application::get().getPresetManager()->getEditBuffer();
  auto vg = Application::get().getHWUI()->getCurrentVoiceGroup();

  for(const auto i : parameters)
  {
    auto signalFlowIndicator = ParameterDB::get().getSignalPathIndication(i);

    Parameter* parameter = nullptr;

    parameter = editBuffer->findParameterByID({ i, vg });

    if(!parameter)
      parameter = editBuffer->findParameterByID({ i, VoiceGroup::Global });

    if(parameter != nullptr)
    {
      if(isSpecialCaseParameterForActivityOverview(parameter))
      {
        if(specialCaseParameterMatches(parameter))
        {
          states[(int) buttonId] = TwoStateLED::ON;
          break;
        }
      }
      else if(auto mc = dynamic_cast<MacroControlParameter*>(parameter))
      {
        if(!mc->getTargets().empty())
        {
          states[(int) buttonId] = TwoStateLED::ON;
          break;
        }
      }
      else if(signalFlowIndicator != invalidSignalFlowIndicator)
      {
        if(parameter->getControlPositionValue() != signalFlowIndicator)
        {
          states[(int) buttonId] = TwoStateLED::ON;
          break;
        }
      }
    }
  }
}

bool PanelUnitPresetMode::specialCaseParameterMatches(Parameter* selParam)
{
  auto eb = Application::get().getPresetManager()->getEditBuffer();
  if(selParam)
  {
    switch(selParam->getID().getNumber())
    {
      //Env A/B Sustain
      case 8:
      {
        auto splitLevel = eb->findParameterByID({ 332, selParam->getVoiceGroup() });
        return splitLevel->getControlPositionValue() != 0 || selParam->getControlPositionValue() > 0;
      }
      case 27:
      {
        auto splitLevel = eb->findParameterByID({ 338, selParam->getVoiceGroup() });
        return splitLevel->getControlPositionValue() != 0 || selParam->getControlPositionValue() > 0;
      }
      //Flanger Mix
      case 223:
      {
        auto tremolo = eb->findParameterByID({ 389, selParam->getVoiceGroup() });
        return tremolo->getControlPositionValue() > 0 || selParam->getControlPositionValue() != 0;
      }
      //Echo Mix
      case 233:
      {
        auto echoSend = eb->findParameterByID({ 342, selParam->getVoiceGroup() });
        return echoSend->getControlPositionValue() > 0 && selParam->getControlPositionValue() > 0;
      }
      //Reverb Mix
      case 241:
      {
        auto reverbSend = eb->findParameterByID({ 344, selParam->getVoiceGroup() });
        return reverbSend->getControlPositionValue() > 0 && selParam->getControlPositionValue() > 0;
      }
      default:
        return false;
    }
  }
  return false;
}

void PanelUnitPresetMode::applyStateToLeds(std::array<TwoStateLED::LedState, numLeds>& states)
{
  auto& panelUnit = Application::get().getHWUI()->getPanelUnit();
  for(int i = 0; i < numLeds; i++)
  {
    panelUnit.getLED((Buttons) i)->setState(states[i]);
  }
}

bool PanelUnitPresetMode::isSpecialCaseParameterForActivityOverview(Parameter* selParam) const
{
  if(selParam)
  {
    for(auto id : { 8, 27, 223, 233, 241 })
    {
      if(id == selParam->getID().getNumber())
      {
        return true;
      }
    }
  }
  return false;
}

PanelUnitSoundMode::PanelUnitSoundMode()
{
}

void PanelUnitSoundMode::setup()
{
  PanelUnitPresetMode::setup();

  removeButtonConnection(Buttons::BUTTON_SOUND);

  setupButtonConnection(Buttons::BUTTON_SOUND, [&](Buttons button, ButtonModifiers modifiers, bool state) {
    if(state)
    {
      auto focusAndMode = Application::get().getHWUI()->getFocusAndMode();
      if(focusAndMode.focus == UIFocus::Sound)
        if(focusAndMode.mode == UIMode::Edit || focusAndMode.detail == UIDetail::Voices)
          Application::get().getHWUI()->setFocusAndMode({ UIFocus::Sound, UIMode::Select, UIDetail::Init });
        else
          Application::get().getHWUI()->setFocusAndMode({ UIFocus::Parameters, UIMode::Select });
      else
        Application::get().getHWUI()->undoableSetFocusAndMode(UIFocus::Sound);
    }

    return true;
  });
}
