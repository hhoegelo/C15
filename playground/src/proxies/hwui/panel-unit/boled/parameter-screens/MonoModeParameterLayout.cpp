#include <Application.h>
#include <presets/PresetManager.h>
#include <presets/EditBuffer.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/MonoParameterCarousel.h>
#include "MonoModeParameterLayout.h"
#include <proxies/hwui/controls/Button.h>

Parameter *MonoModeParameterLayout::getCurrentParameter() const
{
  auto eb = Application::get().getPresetManager()->getEditBuffer();

  if(eb->getType() == SoundType::Split)
    return eb->getSelected();
  else
    return eb->getSelected(VoiceGroup::I);
}

MonoModeParameterLayout::MonoModeParameterLayout()
    : UnmodulateableParameterSelectLayout2()
{
}

void MonoModeParameterLayout::init() {
  UnmodulateableParameterSelectLayout2::init();

  for(auto& c: getControls<Button>()) {
    if(c->getButtonPos(Buttons::BUTTON_C) == c->getPosition()) {
      c->setText({"^", 0});
    }
  }
}

Carousel *MonoModeParameterLayout::createCarousel(const Rect &rect)
{
  return new MonoParameterCarousel(rect);
}

bool MonoModeParameterLayout::onButton(Buttons i, bool down, ButtonModifiers modifiers)
{
  if(down && i == Buttons::BUTTON_C)
  {
    Application::get().getHWUI()->setFocusAndMode({ UIFocus::Sound, UIMode::Select, UIDetail::Voices });
    return true;
  }

  return ParameterSelectLayout2::onButton(i, down, modifiers);
}
