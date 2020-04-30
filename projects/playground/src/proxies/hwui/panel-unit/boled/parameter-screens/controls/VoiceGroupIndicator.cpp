#include <Application.h>
#include <presets/PresetManager.h>
#include <presets/EditBuffer.h>
#include <proxies/hwui/HWUI.h>
#include "VoiceGroupIndicator.h"
#include <proxies/hwui/FrameBuffer.h>
#include <proxies/hwui/controls/SwitchVoiceGroupButton.h>
#include <groups/MacroControlsGroup.h>

VoiceGroupIndicator::VoiceGroupIndicator(const Rect& r)
    : Control(r)
{
  Application::get().getPresetManager()->getEditBuffer()->onSoundTypeChanged(
      sigc::mem_fun(this, &VoiceGroupIndicator::onSoundTypeChanged));

  auto eb = Application::get().getPresetManager()->getEditBuffer();

  eb->onSelectionChanged(sigc::mem_fun(this, &VoiceGroupIndicator::onParameterSelectionChanged));

  Application::get().getHWUI()->onCurrentVoiceGroupChanged(
      sigc::mem_fun(this, &VoiceGroupIndicator::onVoiceGroupSelectionChanged));
}

VoiceGroupIndicator::~VoiceGroupIndicator()
{
  m_parameterChanged.disconnect();
}

bool VoiceGroupIndicator::redraw(FrameBuffer& fb)
{
  if(m_shouldDraw)
  {
    if(m_currentSoundType == SoundType::Split)
      return drawSplit(fb);
    else if(m_currentSoundType == SoundType::Layer)
      return drawLayer(fb);
  }

  return true;
}

bool VoiceGroupIndicator::drawLayer(FrameBuffer& fb)
{
  auto absPos = getPosition();
  fb.setColor(m_selectedVoiceGroup == VoiceGroup::I ? FrameBufferColors::C255 : FrameBufferColors::C128);
  fb.fillRect(Rect(absPos.getLeft(), absPos.getTop(), 11, 5));
  fb.setColor(m_selectedVoiceGroup == VoiceGroup::II ? FrameBufferColors::C255 : FrameBufferColors::C128);
  fb.fillRect(Rect(absPos.getLeft(), absPos.getTop() + 6, 11, 5));
  return true;
}

bool VoiceGroupIndicator::drawSplit(FrameBuffer& fb)
{
  auto absPos = getPosition();
  fb.setColor(m_selectedVoiceGroup == VoiceGroup::I ? FrameBufferColors::C255 : FrameBufferColors::C128);
  fb.fillRect(Rect(absPos.getLeft(), absPos.getTop(), 5, 11));
  fb.setColor(m_selectedVoiceGroup == VoiceGroup::II ? FrameBufferColors::C255 : FrameBufferColors::C128);
  fb.fillRect(Rect(absPos.getLeft() + 6, absPos.getTop(), 5, 11));
  return true;
}

void VoiceGroupIndicator::onSoundTypeChanged()
{
  m_currentSoundType = Application::get().getPresetManager()->getEditBuffer()->getType();
  if(m_currentSoundType == SoundType::Single)
    m_shouldDraw = false;
  setDirty();
}

void VoiceGroupIndicator::onParameterChanged(const Parameter* parameter)
{
  auto isSplit = parameter->getParentGroup()->getID().getName() == "Split";
  auto isMC = MacroControlsGroup::isMacroControl(parameter->getID().getNumber());

  m_shouldDraw
      = SwitchVoiceGroupButton::allowToggling(parameter, Application::get().getPresetManager()->getEditBuffer());
  m_shouldDraw |= isMC;
  m_shouldDraw |= isSplit;

  if(isSplit || isMC)
    m_selectedVoiceGroup = Application::get().getHWUI()->getCurrentVoiceGroup();
  else
    m_selectedVoiceGroup = parameter->getID().getVoiceGroup();

  onSoundTypeChanged();
  setDirty();
}

void VoiceGroupIndicator::onParameterSelectionChanged(const Parameter* old, const Parameter* newParam)
{
  m_parameterChanged.disconnect();
  if(newParam)
  {
    m_parameterChanged = newParam->onParameterChanged(sigc::mem_fun(this, &VoiceGroupIndicator::onParameterChanged));
  }
}

void VoiceGroupIndicator::onVoiceGroupSelectionChanged(VoiceGroup vg)
{
  m_selectedVoiceGroup = vg;
  setDirty();
}
