#include <Application.h>
#include <glibconfig.h>
#include <glibmm/ustring.h>
#include <groups/MacroControlsGroup.h>
#include <libundo/undo/Scope.h>
#include <libundo/undo/Transaction.h>
#include <libundo/undo/TransactionCreationScope.h>
#include <parameters/ModulateableParameter.h>
#include <parameters/Parameter.h>
#include <presets/EditBuffer.h>
#include <presets/PresetManager.h>
#include <proxies/hwui/buttons.h>
#include <proxies/hwui/controls/ControlOwner.h>
#include <proxies/hwui/controls/Overlay.h>
#include <proxies/hwui/controls/Rect.h>
#include <proxies/hwui/controls/SelectedParameterValue.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/MCAmountButton.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/MCAmountLabel.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/MCPositionButton.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/MCPositionLabel.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/MCSelectButton.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/ModulationCarousel.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/ModulationSourceEnabledDottedLine.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/ModulationSourceLabel.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/ParameterCarousel.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/ParameterNameLabel.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/SelectedParameterBarSlider.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/SelectedParameterKnubbelSlider.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/SelectedParameterModAmount.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/SelectedParamsMacroControlSlider.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/ModulateableParameterLayouts.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/ParameterEditButtonMenu.h>

ModulateableParameterLayout2::ModulateableParameterLayout2()
{
}

void ModulateableParameterLayout2::addModAmountSliders(ControlOwner *o)
{
  o->addControl(new SelectedParameterModAmount(Rect(BIG_SLIDER_X + 1, 23, BIG_SLIDER_WIDTH - 2, 1)));
  o->addControl(new SelectedParameterModAmount(Rect(BIG_SLIDER_X + 1, 30, BIG_SLIDER_WIDTH - 2, 1)));
}

bool ModulateableParameterLayout2::switchToNormalMode()
{
  return false;
}

ModulateableParameterSelectLayout2::ModulateableParameterSelectLayout2()
    : virtual_base()
    , super1()
    , super2()
{
  addControl(new MCPositionButton(BUTTON_A));
  addControl(new MCSelectButton(BUTTON_B));
  addControl(new MCAmountButton(BUTTON_C));

  m_modeOverlay = addControl(new Overlay(Rect(0, 0, 256, 64)));

  setMode(Mode::ParameterValue);

  Application::get().getPresetManager()->getEditBuffer()->onSelectionChanged(
      sigc::mem_fun(this, &ModulateableParameterSelectLayout2::onSelectedParameterChanged));
}

void ModulateableParameterSelectLayout2::onSelectedParameterChanged(Parameter *, Parameter *newParam)
{
  m_paramConnection.disconnect();

  if(newParam)
    m_paramConnection = newParam->onParameterChanged(
        sigc::mem_fun(this, &ModulateableParameterSelectLayout2::onCurrentParameterChanged));
}

void ModulateableParameterSelectLayout2::onCurrentParameterChanged(const Parameter *p)
{
  fixModeIfNecessary(p);
}

void ModulateableParameterSelectLayout2::fixModeIfNecessary(const Parameter *p)
{
  auto isModulateable = dynamic_cast<const ModulateableParameter *>(p) != nullptr;
  auto isModulated = isModulateable
      && dynamic_cast<const ModulateableParameter *>(p)->getModulationSource()
          != ModulateableParameter::ModulationSource::NONE;

  switch(m_mode)
  {
    case Mode::ParameterValue:
      return;

    case Mode::MacroControlPosition:
    case Mode::MacroControlAmount:
    case Mode::CarouselUpperBound:
    case Mode::CarouselParameterValue:
    case Mode::CarouselLowerBound:
      if(!isModulated)
        setMode(Mode::ParameterValue);
      break;

    case Mode::MacroControlSelection:
      if(!isModulateable)
        setMode(Mode::ParameterValue);
      break;
  }
}

void ModulateableParameterSelectLayout2::copyFrom(Layout *other)
{
  if(auto p = dynamic_cast<ModulateableParameterSelectLayout2 *>(other))
    setMode(p->m_mode);

  super1::copyFrom(other);
  super2::copyFrom(other);
}

void ModulateableParameterSelectLayout2::init()
{
  super1::init();
  super2::init();
}

bool ModulateableParameterSelectLayout2::onButton(int i, bool down, ButtonModifiers modifiers)
{
  if(auto m = dynamic_cast<ModulationCarousel *>(getCarousel()))
  {
    if(m->onButton(i, down, modifiers))
      return true;
  }

  if(down)
  {
    switch(i)
    {
      case BUTTON_A:
        if(hasModulationSource())
          toggleMode(Mode::MacroControlPosition);
        return true;

      case BUTTON_B:
        toggleMode(Mode::MacroControlSelection);
        return true;

      case BUTTON_C:
        if(hasModulationSource())
          toggleMode(Mode::MacroControlAmount);
        return true;

      case BUTTON_D:
        if(auto m = dynamic_cast<ModulationCarousel *>(getCarousel()))
        {
          if(m_mode == Mode::CarouselUpperBound)
            toggleMode(Mode::CarouselParameterValue);
          else if(m_mode == Mode::CarouselParameterValue)
            toggleMode(Mode::CarouselLowerBound);
          else
            toggleMode(Mode::CarouselUpperBound);
          return true;
        }
        break;
    }
  }

  if(super1::onButton(i, down, modifiers))
    return true;

  return super2::onButton(i, down, modifiers);
}

Parameter *ModulateableParameterSelectLayout2::getCurrentEditParameter() const
{
  if(m_mode == Mode::MacroControlPosition)
  {
    if(auto p = dynamic_cast<ModulateableParameter *>(getCurrentParameter()))
    {
      auto src = p->getModulationSource();
      uint16_t srcParamID = MacroControlsGroup::modSrcToParamID(src);
      return Application::get().getPresetManager()->getEditBuffer()->findParameterByID(srcParamID);
    }
  }

  return super1::getCurrentEditParameter();
}

bool ModulateableParameterSelectLayout2::hasModulationSource() const
{
  if(auto p = dynamic_cast<ModulateableParameter *>(getCurrentParameter()))
  {
    auto src = p->getModulationSource();
    return src != ModulateableParameter::NONE;
  }
  return false;
}

bool ModulateableParameterSelectLayout2::onRotary(int inc, ButtonModifiers modifiers)
{
  switch(m_mode)
  {
    case Mode::MacroControlSelection:
      if(auto p = dynamic_cast<ModulateableParameter *>(getCurrentParameter()))
      {
        auto scope = p->getUndoScope().startTransaction("Set MC Select for '%0'", p->getLongName());
        p->undoableIncrementMCSelect(scope->getTransaction(), inc);
      }

      return true;

    case Mode::MacroControlAmount:
      if(auto p = dynamic_cast<ModulateableParameter *>(getCurrentParameter()))
      {
        auto scope = p->getUndoScope().startContinuousTransaction(p->getAmountCookie(), "Set MC Amount for '%0'",
                                                                  p->getGroupAndParameterName());
        p->undoableIncrementMCAmount(scope->getTransaction(), inc, modifiers);
      }

      return true;

    case Mode::ParameterValue:
    case Mode::MacroControlPosition:
      break;

    case Mode::CarouselLowerBound:
    case Mode::CarouselUpperBound:
    case Mode::CarouselParameterValue:
      if(auto m = dynamic_cast<ModulationCarousel *>(getCarousel()))
      {
        return m->onRotary(inc, modifiers);
      }
      break;
  }

  if(super1::onRotary(inc, modifiers))
    return true;

  return super2::onRotary(inc, modifiers);
}

void ModulateableParameterSelectLayout2::setDefault()
{
  switch(m_mode)
  {
    case Mode::MacroControlSelection:
      if(auto p = dynamic_cast<ModulateableParameter *>(getCurrentParameter()))
      {
        auto scope = p->getUndoScope().startTransaction("Set MC Select for '%0'", p->getLongName());
        p->undoableSelectModSource(scope->getTransaction(), 0);
      }
      return;

    case Mode::MacroControlAmount:
      if(auto p = dynamic_cast<ModulateableParameter *>(getCurrentParameter()))
        p->undoableSetMCAmountToDefault();
      return;

    case Mode::ParameterValue:
    case Mode::MacroControlPosition:
      super1::setDefault();
      super2::setDefault();
      break;
  }
}

bool ModulateableParameterSelectLayout2::switchToNormalMode()
{
  if(m_mode == Mode::ParameterValue)
    return false;

  setMode(Mode::ParameterValue);
  return true;
}

void ModulateableParameterSelectLayout2::toggleMode(Mode desiredMode)
{
  if(m_mode == desiredMode)
    setMode(Mode::ParameterValue);
  else
    setMode(desiredMode);
}

void ModulateableParameterSelectLayout2::installMcAmountScreen()
{
  setMode(Mode::MacroControlAmount);
}

void ModulateableParameterSelectLayout2::setMode(Mode desiredMode)
{
  m_mode = desiredMode;

  m_modeOverlay->clear();
  noHighlight();
  setDirty();

  if(auto p = getCurrentParameter())
  {
    if(p->getVisualizationStyle() == Parameter::VisualizationStyle::Dot)
      m_modeOverlay->addControl(new SelectedParameterKnubbelSlider(Rect(BIG_SLIDER_X, 24, BIG_SLIDER_WIDTH, 6)));
    else
      m_modeOverlay->addControl(new SelectedParameterBarSlider(Rect(BIG_SLIDER_X, 24, BIG_SLIDER_WIDTH, 6)));
  }

  switch(m_mode)
  {
    case Mode::ParameterValue:
      addModAmountSliders(m_modeOverlay);
      m_modeOverlay->addControl(new SelectedParameterValue(Rect(90, 33, 76, 12)));
      m_modeOverlay->addControl(new ModulationSourceLabel(Rect(0, 21, 55, 12), Font::Justification::Right));
      m_modeOverlay->addControl(new ModulationSourceEnabledDottedLine(Rect(57, 27, 17, 1)));

      setCarousel(new ParameterCarousel(Rect(195, 0, 58, 62)));

      m_modeOverlay->highlight<SelectedParameterValue>();
      m_modeOverlay->highlight<SelectedParameterBarSlider>();
      m_modeOverlay->highlight<SelectedParameterKnubbelSlider>();
      highlight<ParameterNameLabel>();
      highlight<ParameterCarousel>();
      break;

    case Mode::MacroControlPosition:
      addModAmountSliders(m_modeOverlay);
      m_modeOverlay->addControl(new ModulationSourceEnabledDottedLine(Rect(60, 27, 13, 1)));
      m_modeOverlay->addControl(new SelectedParamsMacroControlSlider(Rect(8, 25, 48, 4)));
      m_modeOverlay->addControl(new ModulationSourceLabel(Rect(64, BUTTON_VALUE_Y_POSITION, 64, 12)));
      m_modeOverlay->addControl(new MCPositionLabel(Rect(0, BUTTON_VALUE_Y_POSITION, 64, 12)));
      m_modeOverlay->addControl(new MCAmountLabel(Rect(131, BUTTON_VALUE_Y_POSITION, 58, 12)));

      setCarousel(new ModulationCarousel(ModulationCarousel::Mode::None, Rect(195, 1, 58, 62)));

      m_modeOverlay->highlight<SelectedParamsMacroControlSlider>();
      m_modeOverlay->highlight<MCPositionLabel>();
      highlight<MCPositionButton>();
      break;

    case Mode::MacroControlSelection:
      addModAmountSliders(m_modeOverlay);
      m_modeOverlay->addControl(new ModulationSourceLabel(Rect(64, BUTTON_VALUE_Y_POSITION, 64, 12)));
      m_modeOverlay->addControl(new MCPositionLabel(Rect(0, BUTTON_VALUE_Y_POSITION, 64, 12)));
      m_modeOverlay->addControl(new MCAmountLabel(Rect(131, BUTTON_VALUE_Y_POSITION, 58, 12)));
      m_modeOverlay->addControl(new ModulationSourceEnabledDottedLine(Rect(60, 27, 13, 1)));
      m_modeOverlay->addControl(new SelectedParamsMacroControlSlider(Rect(8, 25, 48, 4)));

      setCarousel(new ModulationCarousel(ModulationCarousel::Mode::None, Rect(195, 1, 58, 62)));

      m_modeOverlay->highlight<ModulationSourceLabel>();
      highlight<MCSelectButton>();
      break;

    case Mode::MacroControlAmount:
      addModAmountSliders(m_modeOverlay);
      m_modeOverlay->addControl(new ModulationSourceLabel(Rect(64, BUTTON_VALUE_Y_POSITION, 64, 12)));
      m_modeOverlay->addControl(new MCPositionLabel(Rect(0, BUTTON_VALUE_Y_POSITION, 64, 12)));
      m_modeOverlay->addControl(new MCAmountLabel(Rect(131, BUTTON_VALUE_Y_POSITION, 58, 12)));
      m_modeOverlay->addControl(new ModulationSourceEnabledDottedLine(Rect(60, 27, 13, 1)));
      m_modeOverlay->addControl(new SelectedParamsMacroControlSlider(Rect(8, 25, 48, 4)));

      setCarousel(new ModulationCarousel(ModulationCarousel::Mode::None, Rect(195, 1, 58, 62)));

      m_modeOverlay->highlight<MCAmountLabel>();
      highlight<MCAmountButton>();
      break;

    case Mode::CarouselUpperBound:
      addModAmountSliders(m_modeOverlay);
      m_modeOverlay->addControl(new ModulationSourceLabel(Rect(64, BUTTON_VALUE_Y_POSITION, 64, 12)));
      m_modeOverlay->addControl(new MCPositionLabel(Rect(0, BUTTON_VALUE_Y_POSITION, 64, 12)));
      m_modeOverlay->addControl(new MCAmountLabel(Rect(131, BUTTON_VALUE_Y_POSITION, 58, 12)));
      m_modeOverlay->addControl(new ModulationSourceEnabledDottedLine(Rect(60, 27, 13, 1)));
      m_modeOverlay->addControl(new SelectedParamsMacroControlSlider(Rect(8, 25, 48, 4)));
      setCarousel(new ModulationCarousel(ModulationCarousel::Mode::UpperBound, Rect(195, 1, 58, 62)));
      break;

    case Mode::CarouselParameterValue:
      addModAmountSliders(m_modeOverlay);
      m_modeOverlay->addControl(new ModulationSourceLabel(Rect(64, BUTTON_VALUE_Y_POSITION, 64, 12)));
      m_modeOverlay->addControl(new MCPositionLabel(Rect(0, BUTTON_VALUE_Y_POSITION, 64, 12)));
      m_modeOverlay->addControl(new MCAmountLabel(Rect(131, BUTTON_VALUE_Y_POSITION, 58, 12)));
      m_modeOverlay->addControl(new ModulationSourceEnabledDottedLine(Rect(60, 27, 13, 1)));
      m_modeOverlay->addControl(new SelectedParamsMacroControlSlider(Rect(8, 25, 48, 4)));
      setCarousel(new ModulationCarousel(ModulationCarousel::Mode::ParameterValue, Rect(195, 1, 58, 62)));

      break;

    case Mode::CarouselLowerBound:
      addModAmountSliders(m_modeOverlay);
      m_modeOverlay->addControl(new ModulationSourceLabel(Rect(64, BUTTON_VALUE_Y_POSITION, 64, 12)));
      m_modeOverlay->addControl(new MCPositionLabel(Rect(0, BUTTON_VALUE_Y_POSITION, 64, 12)));
      m_modeOverlay->addControl(new MCAmountLabel(Rect(131, BUTTON_VALUE_Y_POSITION, 58, 12)));
      m_modeOverlay->addControl(new ModulationSourceEnabledDottedLine(Rect(60, 27, 13, 1)));
      m_modeOverlay->addControl(new SelectedParamsMacroControlSlider(Rect(8, 25, 48, 4)));
      setCarousel(new ModulationCarousel(ModulationCarousel::Mode::LowerBound, Rect(195, 1, 58, 62)));
      break;
  }

  setAllDirty();
}

ModulateableParameterEditLayout2::ModulateableParameterEditLayout2()
    : super1()
    , super2()
{
  addControl(new Button("", BUTTON_A));
  addControl(new Button("", BUTTON_B));
  addControl(new Button("", BUTTON_C));

  if(auto p = getCurrentParameter())
  {
    if(p->getVisualizationStyle() == Parameter::VisualizationStyle::Dot)
      addControl(new SelectedParameterKnubbelSlider(Rect(BIG_SLIDER_X, 24, BIG_SLIDER_WIDTH, 6)));
    else
      addControl(new SelectedParameterBarSlider(Rect(BIG_SLIDER_X, 24, BIG_SLIDER_WIDTH, 6)));
  }

  addModAmountSliders(this);
  addControl(new SelectedParameterValue(Rect(90, 33, 76, 12)));

  highlight<SelectedParameterValue>();
  highlight<SelectedParameterBarSlider>();
  highlight<SelectedParameterKnubbelSlider>();
  highlight<ParameterNameLabel>();
}

ButtonMenu *ModulateableParameterEditLayout2::createMenu(const Rect &rect)
{
  return new ParameterEditButtonMenu(rect);
}
