#include "SwitchVoiceGroupButton.h"
#include <Application.h>
#include <presets/PresetManager.h>
#include <presets/EditBuffer.h>
#include <proxies/hwui/HWUI.h>
#include <parameters/mono-mode-parameters/UnmodulateableMonoParameter.h>
#include <parameters/mono-mode-parameters/ModulateableMonoParameter.h>
#include <groups/MonoGroup.h>
#include <parameters/mono-mode-parameters/MonoGlideTimeParameter.h>
#include <proxies/hwui/HWUI.h>
#include <groups/UnisonGroup.h>

SwitchVoiceGroupButton::SwitchVoiceGroupButton(Buttons pos)
    : Button(getTextFor(Application::get().getHWUI()->getCurrentVoiceGroup()), pos)
{
  Application::get().getPresetManager()->getEditBuffer()->onSelectionChanged(
      sigc::mem_fun(this, &SwitchVoiceGroupButton::onParameterSelectionChanged));

  Application::get().getHWUI()->onCurrentVoiceGroupChanged(
      sigc::mem_fun(this, &SwitchVoiceGroupButton::onVoiceGroupChanged));
}

Glib::ustring SwitchVoiceGroupButton::getTextFor(VoiceGroup vg)
{
  if(vg == VoiceGroup::Global)
    return "";
  if(vg == VoiceGroup::I)
    return "Select II";
  else
    return "Select I";
}

void SwitchVoiceGroupButton::rebuild()
{
  auto eb = Application::get().getPresetManager()->getEditBuffer();
  auto ebType = eb->getType();
  auto selected = eb->getSelected();
  auto selectedVoiceGroup = Application::get().getHWUI()->getCurrentVoiceGroup();

  if(EditBuffer::isDualParameterForSoundType(selected, ebType))
    setText({ getTextFor(selectedVoiceGroup), 0 });
  else
    setText({ "", 0 });
}

void SwitchVoiceGroupButton::onParameterSelectionChanged(Parameter* oldSelected, Parameter* newSelection)
{
  rebuild();
}

void SwitchVoiceGroupButton::onVoiceGroupChanged(VoiceGroup newVoiceGroup)
{
  rebuild();
}

bool SwitchVoiceGroupButton::toggleVoiceGroup()
{
  auto pm = Application::get().getPresetManager();
  auto eb = pm->getEditBuffer();
  auto selected = eb->getSelected();

  auto didToggleAction = false;

  if(allowToggling(selected, eb))
  {
    auto otherVG = selected->getVoiceGroup() == VoiceGroup::I ? VoiceGroup::II : VoiceGroup::I;
    if(auto other = eb->findParameterByID({ selected->getID().getNumber(), otherVG }))
    {
      auto scope = pm->getUndoScope().startContinuousTransaction(&other, std::chrono::hours(1), "Select '%0'",
                                                                 other->getGroupAndParameterNameWithVoiceGroup());
      Application::get().getHWUI()->toggleCurrentVoiceGroupAndUpdateParameterSelection(scope->getTransaction());
      didToggleAction = true;
    }
  }

  if(dynamic_cast<const SplitPointParameter*>(selected))
  {
    Application::get().getHWUI()->toggleCurrentVoiceGroup();
    didToggleAction = true;
  }

  return didToggleAction;
}

bool SwitchVoiceGroupButton::allowToggling(const Parameter* selected, const EditBuffer* editBuffer)
{
  auto hasCounterPart = selected->getVoiceGroup() != VoiceGroup::Global;
  auto layerAndGroupAllowToggling
      = ((editBuffer->getType() == SoundType::Layer)
         && (!MonoGroup::isMonoParameter(selected) && !UnisonGroup::isUnisonParameter(selected)))
      || (editBuffer->getType() != SoundType::Layer);

  return hasCounterPart && layerAndGroupAllowToggling;
}
