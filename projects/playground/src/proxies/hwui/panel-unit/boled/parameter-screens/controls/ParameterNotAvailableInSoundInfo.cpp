#include "ParameterNotAvailableInSoundInfo.h"
#include "ParameterCarousel.h"
#include <Application.h>
#include <presets/PresetManager.h>
#include <presets/EditBuffer.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/ParameterLayout.h>
#include <proxies/hwui/FrameBuffer.h>
#include <proxies/hwui/HWUI.h>
#include <sigc++/adaptors/hide.h>
#include <proxies/hwui/controls/CenterAlignedLabel.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/controls/ModuleCaption.h>
#include <proxies/hwui/controls/Button.h>
#include <proxies/hwui/Oleds.h>

namespace detail
{
  class BoldCenterAlignedLabel : public CenterAlignedLabel
  {
   public:
    using CenterAlignedLabel::CenterAlignedLabel;
    std::shared_ptr<Font> getFont() const override
    {
      return Oleds::get().getFont("Emphase-8-Bold", getFontHeight());
    }
  };
}

ParameterNotAvailableInSoundInfo::ParameterNotAvailableInSoundInfo(const Rect &r, ParameterLayout2 *parent)
    : ControlWithChildren(r)
    , m_parent { parent }
{
  auto eb = Application::get().getPresetManager()->getEditBuffer();

  m_parameterSelectionConnection
      = eb->onSelectionChanged(sigc::mem_fun(this, &ParameterNotAvailableInSoundInfo::onSelectionChanged));

  m_soundTypeConnection
      = eb->onSoundTypeChanged(sigc::hide(sigc::mem_fun(this, &ParameterNotAvailableInSoundInfo::onSoundTypeChanged)));

  m_parameterNameLabel
      = addControl(new detail::BoldCenterAlignedLabel(eb->getSelected()->getLongName(), { 0, 8, 128, 10 }));
  m_parameterNameLabel->setHighlight(true);
  addControl(new CenterAlignedLabel("Only available with", { 0, 20, 128, 10 }))->setHighlight(true);
  addControl(new CenterAlignedLabel("Layer Sounds", { 0, 32, 128, 10 }))->setHighlight(true);
}

void ParameterNotAvailableInSoundInfo::setBackgroundColor(FrameBuffer &fb) const
{
  fb.setColor(FrameBufferColors::C43);
}

void ParameterNotAvailableInSoundInfo::onSelectionChanged(const Parameter *old, const Parameter *newParam)
{
  auto vis = !ParameterLayout2::isParameterAvailableInSoundType(newParam);
  m_parameterNameLabel->setText({ newParam->getLongName(), 0 });
  setVisible(vis);
}

void ParameterNotAvailableInSoundInfo::onSoundTypeChanged()
{
  auto eb = Application::get().getPresetManager()->getEditBuffer();
  auto current = eb->getSelected();
  const auto vis = !ParameterLayout2::isParameterAvailableInSoundType(current, eb);
  m_parameterNameLabel->setText({ current->getLongName(), 0 });
  setVisible(vis);
}

void ParameterNotAvailableInSoundInfo::setVisible(bool b)
{
  for(auto &l : getControls<Label>())
  {
    l->setHighlight(true);
  }

  for(auto &c : m_parent->getControls())
  {
    if(c.get() != this && dynamic_cast<const ModuleCaption *>(c.get()) == nullptr
       && dynamic_cast<const Button *>(c.get()) == nullptr
       && dynamic_cast<const ParameterCarousel *>(c.get()) == nullptr)
    {
      c->setVisible(!b);
    }
    else if(auto button = dynamic_cast<Button *>(c.get()))
    {
      button->blind(b);
    }
  }

  Control::setVisible(b);
}
