#include "Carousel.h"
#include "Application.h"
#include "presets/PresetManager.h"
#include "presets/EditBuffer.h"
#include <sigc++/sigc++.h>

Carousel::Carousel(const Rect &pos)
    : super(pos)
{
  Application::get().getPresetManager()->getEditBuffer()->onSelectionChanged(
      sigc::hide<0>(sigc::mem_fun(this, &Carousel::setup)));

  Application::get().getPresetManager()->getEditBuffer()->onSoundTypeChanged(
      sigc::mem_fun(this, &Carousel::setupSelected), false);
}

Carousel::~Carousel() = default;

void Carousel::antiTurn()
{
  turn();
}

void Carousel::setupSelected()
{
  auto eb = Application::get().getPresetManager()->getEditBuffer();
  setup(eb->getSelected());
}
