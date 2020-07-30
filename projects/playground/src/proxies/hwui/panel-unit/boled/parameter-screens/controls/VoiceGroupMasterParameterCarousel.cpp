#include "VoiceGroupMasterParameterCarousel.h"
#include "NeverHighlitButton.h"
#include "MiniParameter.h"
#include <Application.h>
#include <presets/PresetManager.h>
#include <presets/EditBuffer.h>
#include <proxies/hwui/HWUI.h>
#include <parameters/voice-group-master-group/VoiceGroupMasterModulateableParameter.h>
#include <sigc++/sigc++.h>

VoiceGroupMasterParameterCarousel::VoiceGroupMasterParameterCarousel(const Rect &r)
    : ParameterCarousel(r)
{
  auto eb = Application::get().getPresetManager()->getEditBuffer();
  m_editbufferConnection = eb->onChange(sigc::mem_fun(this, &VoiceGroupMasterParameterCarousel::rebuild));

  m_selectConnection
      = eb->onCurrentVoiceGroupChanged(sigc::hide(sigc::mem_fun(this, &VoiceGroupMasterParameterCarousel::rebuild)));
}

VoiceGroupMasterParameterCarousel::~VoiceGroupMasterParameterCarousel()
{
  m_selectConnection.disconnect();
  m_editbufferConnection.disconnect();
}

void VoiceGroupMasterParameterCarousel::setup(Parameter *selectedParameter)
{
  clear();

  auto vg = Application::get().getPresetManager()->getEditBuffer()->getCurrentHWUIVoiceGroup();

  switch(Application::get().getPresetManager()->getEditBuffer()->getType())
  {
    default:
    case SoundType::Single:
      setupMasterParameters({ { 358, vg }, { 360, vg } });
      break;
    case SoundType::Split:
      setupMasterParameters({ { 358, vg }, { 360, vg }, { 356, VoiceGroup::Global } });
      break;
    case SoundType::Layer:
      setupMasterParameters({ { 358, vg }, { 360, vg }, { 396, vg }, { 397, vg } });
      break;
  }

  if(getNumChildren() == 0)
    addControl(new NeverHighlitButton("", Buttons::BUTTON_D));
  else
    setHighlight(true);

  setDirty();
}

void VoiceGroupMasterParameterCarousel::rebuild()
{
  auto s = Application::get().getPresetManager()->getEditBuffer()->getSelected();
  setup(s);
}

void VoiceGroupMasterParameterCarousel::setupMasterParameters(const std::vector<ParameterId> &parameters)
{
  const int ySpacing = 3;
  const int miniParamHeight = 12;
  const int miniParamWidth = 56;
  const auto numMissing = 4 - parameters.size();
  auto yPos = ySpacing + (numMissing * (miniParamHeight + ySpacing));

  const auto selected = Application::get().getPresetManager()->getEditBuffer()->getSelected();

  for(const auto &p : parameters)
  {
    auto param = Application::get().getPresetManager()->getEditBuffer()->findParameterByID(p);
    if(param)
    {
      auto miniParam = new MiniParameter(param, Rect(0, yPos, miniParamWidth, miniParamHeight));
      miniParam->setSelected(param == selected);
      addControl(miniParam);
      yPos += ySpacing;
      yPos += miniParamHeight;
    }
  }
}
