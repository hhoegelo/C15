#include "ModulationModeButton.h"
#include "Application.h"
#include "parameters/ModulateableParameter.h"
#include "presets/PresetManager.h"
#include "presets/EditBuffer.h"

ModulationModeButton::ModulationModeButton(const Glib::ustring &caption, Buttons id)
    : super(caption, id)
{
  Application::get().getPresetManager()->getEditBuffer()->onSelectionChanged(
      sigc::mem_fun(this, &ModulationModeButton::onParameterSelectionChanged));

  Application::get().getPresetManager()->getEditBuffer()->onSoundTypeChanged(
      sigc::mem_fun(this, &ModulationModeButton::onSoundTypeChanged), false);
}

ModulationModeButton::~ModulationModeButton()
{
}

void ModulationModeButton::onParameterSelectionChanged(Parameter *oldParameter, Parameter *newParameter)
{
  if(newParameter)
  {
    m_param = newParameter;
    m_paramValueConnection.disconnect();
    m_paramValueConnection = newParameter->onParameterChanged(sigc::mem_fun(this, &ModulationModeButton::update));
  }
}

void ModulationModeButton::onSoundTypeChanged()
{
  update(m_param);
}
