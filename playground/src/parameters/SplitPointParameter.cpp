#include "SplitPointParameter.h"

#include <parameters/scale-converters/SplitPointScaleConverter.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/DualVoiceGroupMasterAndSplitPointLayout.h>
#include "groups/ParameterGroup.h"
#include "proxies/hwui/HWUI.h"
#include <proxies/hwui/panel-unit/boled/parameter-screens/ParameterInfoLayout.h>
#include <parameters/scale-converters/dimension/SplitPointDimension.h>

SplitPointParameter::SplitPointParameter(ParameterGroup *group, const ParameterId &id)
    : ModulateableParameter(group, id, ScaleConverter::get<SplitPointScaleConverter>(), 0.5, 59, 59)
{
}

DFBLayout *SplitPointParameter::createLayout(FocusAndMode focusAndMode) const
{
  switch(focusAndMode.mode)
  {
    case UIMode::Info:
      return new ParameterInfoLayout();
    case UIMode::Edit:
      return new ModulateableParameterEditLayout2();
    case UIMode::Select:
    default:
      return new DualVoiceGroupMasterAndSplitPointLayout();
  }
}

std::string SplitPointParameter::getDisplayValue(VoiceGroup vg) const
{
  return SplitPointDimension::stringizeSplitPointDisplay(getValue().getRawValue(), vg);
}

Glib::ustring SplitPointParameter::getDisplayString() const
{
  return getDisplayValue(Application::get().getHWUI()->getCurrentVoiceGroup());
}