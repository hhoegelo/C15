#include "PlayControlParameterLayouts.h"
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/PhysicalControlSlider.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/ModulationRoutersCarousel.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/PhysicalControlBehaviorLabel.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/ParameterNameLabel.h>
#include <proxies/hwui/controls/SelectedParameterValue.h>
#include <proxies/hwui/controls/Button.h>
#include <proxies/hwui/HWUI.h>
#include <Application.h>
#include <presets/PresetManager.h>
#include <presets/EditBuffer.h>
#include <parameters/ParameterAlgorithm.h>
#include <parameters/PhysicalControlParameter.h>
#include <parameters/ModulationRoutingParameter.h>
#include <parameters/MacroControlParameter.h>
#include <parameters/RibbonParameter.h>
#include <groups/HardwareSourcesGroup.h>
#include <groups/MacroControlMappingGroup.h>
#include <device-settings/RibbonRelativeFactor.h>
#include <device-settings/Settings.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/ParameterEditButtonMenu.h>

uint8_t PlayControlParameterLayout2::s_mode = PlayControlParameterLayout2::Mode::ValueEdit;
int PlayControlParameterLayout2::s_instanceCount = 0;

PlayControlParameterLayout2::PlayControlParameterLayout2()
    : super()
{
  addControl(new PhysicalControlSlider(Rect(BIG_SLIDER_X, 24, BIG_SLIDER_WIDTH, 6)));
  addControl(new Button("Select", Buttons::BUTTON_A));

  if(s_instanceCount == 0)
  {
    s_mode = Mode::ValueEdit;
  }

  auto selectButton = addControl(new Button("Select", Buttons::BUTTON_A));
  selectButton->setHighlight(s_mode == Mode::Select);
  s_instanceCount++;
}

void PlayControlParameterLayout2::onInit()
{
  setMode(s_mode);
}

PlayControlParameterLayout2::~PlayControlParameterLayout2()
{
  s_instanceCount--;
}

bool PlayControlParameterLayout2::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  if(down && Buttons::BUTTON_A == i)
  {
    toggleMode(Mode::Select);
    return true;
  }

  return super::onButton(i, down, modifiers);
}

bool PlayControlParameterLayout2::onRotary(int i, ButtonModifiers modifiers)
{
  if(s_mode == Mode::Select)
  {
    auto editBuffer = Application::get().getPresetManager()->getEditBuffer();
    auto hw = dynamic_cast<HardwareSourcesGroup *>(editBuffer->getParameterGroupByID({ "CS", VoiceGroup::Global }));
    auto currentID = getCurrentParameter()->getID();
    auto newParamID = getIdOfAdvancedParameter(hw->getPhysicalControlParameters(), currentID.getNumber(), i);
    editBuffer->undoableSelectParameter({ newParamID, currentID.getVoiceGroup() });
    return true;
  }

  return super::onRotary(i, modifiers);
}

void PlayControlParameterLayout2::toggleMode(uint8_t desiredMode)
{
  if(isModeSupported(desiredMode))
  {
    if(s_mode == desiredMode)
      setMode(Mode::ValueEdit);
    else
      setMode(desiredMode);
  }
}

bool PlayControlParameterLayout2::isModeSupported(uint8_t desiredMode) const
{
  return desiredMode == Mode::ValueEdit || desiredMode == Mode::Select;
}

uint8_t PlayControlParameterLayout2::getMode() const
{
  return s_mode;
}

void PlayControlParameterLayout2::setMode(uint8_t desiredMode)
{
  s_mode = desiredMode;

  noHighlight();
  setDirty();

  switch(s_mode)
  {
    case Mode::ValueEdit:
      highlight<ParameterNameLabel>();
      highlight<PhysicalControlSlider>();
      highlight<SelectedParameterValue>();
      break;

    case Mode::Select:
      highlightButtonWithCaption("Select");
      break;
  }
}

PitchbendParameterLayout2::PitchbendParameterLayout2()
    : virtual_base()
    , super()
{
}

PitchbendParameterSelectLayout2::PitchbendParameterSelectLayout2()
    : virtual_base()
    , PlayControlParameterLayout2()
    , super1()
    , super2()
{
}

bool PitchbendParameterSelectLayout2::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  if(super1::onButton(i, down, modifiers))
    return true;

  return super2::onButton(i, down, modifiers);
}

PitchbendParameterEditLayout2::PitchbendParameterEditLayout2()
    : virtual_base()
    , super1()
    , super2()
{
  addControl(new Button("", Buttons::BUTTON_B));
  addControl(new Button("", Buttons::BUTTON_C));
  addControl(new SelectedParameterValue(Rect(90, 33, 76, 12)));
}

ButtonMenu *PitchbendParameterEditLayout2::createMenu(const Rect &rect)
{
  return new ParameterEditButtonMenu(rect);
}

bool PitchbendParameterEditLayout2::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  if(super1::onButton(i, down, modifiers))
    return true;

  return super2::onButton(i, down, modifiers);
}

AftertouchParameterSelectLayout2::AftertouchParameterSelectLayout2()
    : virtual_base()
    , PlayControlParameterLayout2()
    , super1()
    , super2()
{
}

bool AftertouchParameterSelectLayout2::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  if(super1::onButton(i, down, modifiers))
    return true;

  return super2::onButton(i, down, modifiers);
}

AftertouchParameterLayout2::AftertouchParameterLayout2()
    : virtual_base()
    , super()
{
}

AftertouchParameterEditLayout2::AftertouchParameterEditLayout2()
    : super1()
    , super2()
{
  addControl(new Button("", Buttons::BUTTON_B));
  addControl(new Button("", Buttons::BUTTON_C));
  addControl(new SelectedParameterValue(Rect(90, 33, 76, 12)));
}

ButtonMenu *AftertouchParameterEditLayout2::createMenu(const Rect &rect)
{
  return new ParameterEditButtonMenu(rect);
}

bool AftertouchParameterEditLayout2::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  if(super1::onButton(i, down, modifiers))
    return true;

  return super2::onButton(i, down, modifiers);
}

PedalParameterLayout2::PedalParameterLayout2()
    : virtual_base()
    , super()
{
}

bool PedalParameterLayout2::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  if(down && Buttons::BUTTON_EDIT == i)
  {
    Application::get().getHWUI()->undoableSetFocusAndMode(UIMode::Edit);
    return true;
  }

  return super::onButton(i, down, modifiers);
}

PedalParameterEditLayout2::PedalParameterEditLayout2()
    : virtual_base()
    , PlayControlParameterLayout2()
    , super1()
    , super2()
{
}

bool PedalParameterEditLayout2::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  if(super1::onButton(i, down, modifiers))
    return true;

  return super2::onButton(i, down, modifiers);
}

bool PedalParameterEditLayout2::onRotary(int inc, ButtonModifiers modifiers)
{
  if(super1::onRotary(inc, modifiers))
    return true;

  return super2::onRotary(inc, modifiers);
}

PlayControlParameterSelectLayout2::PlayControlParameterSelectLayout2()
    : virtual_base()
    , super()
{
  addControl(new Button("MC..", Buttons::BUTTON_B));
  addControl(new Button("HW Amt..", Buttons::BUTTON_C));
  addControl(new SelectedParameterValue(Rect(90, 33, 76, 12)));
}

Carousel *PlayControlParameterSelectLayout2::createCarousel(const Rect &rect)
{
  return new ModulationRoutersCarousel(Rect(195, 0, 58, 64));
}

bool PlayControlParameterSelectLayout2::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  if(down)
  {
    auto editBuffer = Application::get().getPresetManager()->getEditBuffer();

    switch(i)
    {
      case Buttons::BUTTON_B:
        if(auto p = dynamic_cast<PhysicalControlParameter *>(getCurrentParameter()))
        {
          auto group = Application::get().getPresetManager()->getEditBuffer()->getParameterGroupByID(
              { "MCM", VoiceGroup::Global });
          auto csGroup = dynamic_cast<MacroControlMappingGroup *>(group);
          auto routingParams = csGroup->getModulationRoutingParametersFor(p);

          for(auto routingParam : routingParams)
          {
            if(routingParam->getID().getNumber() == p->getUiSelectedModulationRouter())
            {
              editBuffer->undoableSelectParameter(routingParam->getTargetParameter()->getID());
              return true;
            }
          }
        }

        return true;

      case Buttons::BUTTON_C:
        if(auto p = dynamic_cast<PhysicalControlParameter *>(getCurrentParameter()))
          editBuffer->undoableSelectParameter(p->getUiSelectedModulationRouter());

        return true;
    }
  }

  return super::onButton(i, down, modifiers);
}

ModuleCaption *PlayControlParameterSelectLayout2::createModuleCaption() const
{
  return ParameterLayout2::createModuleCaption();
}

PlayControlParameterEditLayout2::PlayControlParameterEditLayout2()
    : super()
{
}

bool PlayControlParameterEditLayout2::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  return super::onButton(i, down, modifiers);
}

PlayControlWithBehaviourEditLayout2::PlayControlWithBehaviourEditLayout2()
    : virtual_base()
    , super1()
    , super2()
{
  addControl(new Button("Behavior", Buttons::BUTTON_B));
  addControl(new Button("", Buttons::BUTTON_C));
  addControl(new Button("", Buttons::BUTTON_D));
  addControl(new PhysicalControlBehaviorLabel(Rect(64, BUTTON_VALUE_Y_POSITION, 64, 12)));
}

ButtonMenu *PedalParameterEditLayout2::createMenu(const Rect &rect)
{
  return new ParameterEditButtonMenu(rect);
}

bool PlayControlWithBehaviourEditLayout2::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  if(down && Buttons::BUTTON_B == i)
  {
    toggleMode(Mode::Behaviour);
    return true;
  }

  if(super1::onButton(i, down, modifiers))
    return true;

  return super2::onButton(i, down, modifiers);
}

bool PlayControlWithBehaviourEditLayout2::onRotary(int inc, ButtonModifiers modifiers)
{
  if(getMode() == Mode::Behaviour)
  {
    if(auto p = dynamic_cast<PhysicalControlParameter *>(getCurrentParameter()))
    {
      auto scope = p->getUndoScope().startContinuousTransaction(p, "Set '%0'", p->getGroupAndParameterName());

      int step = inc > 0 ? 1 : -1;

      for(int j = 0; j < std::abs(inc); j++)
      {
        p->undoableStepBehavior(scope->getTransaction(), step);
      }

      return true;
    }
  }

  if(super2::onRotary(inc, modifiers))
    return true;

  return super1::onRotary(inc, modifiers);
}

bool PlayControlWithBehaviourEditLayout2::isModeSupported(uint8_t desiredMode) const
{
  return desiredMode == Mode::Behaviour || super2::isModeSupported(desiredMode);
}

void PlayControlWithBehaviourEditLayout2::setMode(uint8_t desiredMode)
{
  super2::setMode(desiredMode);

  if(desiredMode == Mode::Behaviour)
  {
    highlightButtonWithCaption("Behavior");
  }
}

PedalParameterSelectLayout2::PedalParameterSelectLayout2()
    : virtual_base()
    , PlayControlParameterLayout2()
    , super1()
    , super2()
{
}

bool PedalParameterSelectLayout2::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  if(super1::onButton(i, down, modifiers))
    return true;

  return super2::onButton(i, down, modifiers);
}

RibbonParameterLayout2::RibbonParameterLayout2()
    : virtual_base()
    , super()
{
}

bool RibbonParameterLayout2::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  if(down && Buttons::BUTTON_EDIT == i)
  {
    Application::get().getHWUI()->undoableSetFocusAndMode(UIMode::Edit);
    return true;
  }

  return super::onButton(i, down, modifiers);
}

RibbonParameterEditLayout2::RibbonParameterEditLayout2()
    : virtual_base()
    , PlayControlParameterLayout2()
    , super1()
    , super2()
{
  addControl(new Button("", Buttons::BUTTON_C));
}

ButtonMenu *RibbonParameterEditLayout2::createMenu(const Rect &rect)
{
  return new ParameterEditButtonMenu(rect);
}

bool RibbonParameterEditLayout2::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  if(super1::onButton(i, down, modifiers))
    return true;

  return super2::onButton(i, down, modifiers);
}

bool RibbonParameterEditLayout2::isModeSupported(uint8_t desiredMode) const
{
  return super1::isModeSupported(desiredMode) || super2::isModeSupported(desiredMode);
}

void RibbonParameterEditLayout2::setMode(uint8_t desiredMode)
{
  super1::setMode(desiredMode);
}

bool RibbonParameterEditLayout2::onRotary(int inc, ButtonModifiers modifiers)
{
  if(super1::onRotary(inc, modifiers))
    return true;

  return super2::onRotary(inc, modifiers);
}

RibbonParameterSelectLayout2::RibbonParameterSelectLayout2()
    : virtual_base()
    , PlayControlParameterLayout2()
    , super1()
    , super2()
{
}

bool RibbonParameterSelectLayout2::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  if(super1::onButton(i, down, modifiers))
    return true;

  return super2::onButton(i, down, modifiers);
}
