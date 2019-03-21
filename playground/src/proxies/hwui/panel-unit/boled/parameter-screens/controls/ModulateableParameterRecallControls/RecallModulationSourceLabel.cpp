#include <parameters/ModulateableParameter.h>
#include <Application.h>
#include <presets/PresetManager.h>
#include <presets/PresetParameter.h>
#include <presets/EditBuffer.h>
#include <groups/MacroControlsGroup.h>
#include "RecallModulationSourceLabel.h"

RecallModulationSourceLabel::RecallModulationSourceLabel(Rect pos)
    : Label(pos)
{
  Glib::ustring text = "";
  if(auto modP
     = dynamic_cast<ModulateableParameter*>(Application::get().getPresetManager()->getEditBuffer()->getSelected()))
  {
    uint16_t id = MacroControlsGroup::modSrcToParamID(modP->getOriginalParameter()->getModulationSource());
    if(auto mc = Application::get().getPresetManager()->getEditBuffer()->findParameterByID(id))
    {
      text = mc->getShortName();
    }
    if(text.empty())
      text = "[-]";
    setText(text);
    return;
  }
  setText(text);
}