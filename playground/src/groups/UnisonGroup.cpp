#include "UnisonGroup.h"

#include "parameters/Parameter.h"
#include "parameters/ModulateableParameter.h"

#include "parameters/scale-converters/Linear12CountScaleConverter.h"
#include "parameters/scale-converters/Linear360DegreeScaleConverter.h"
#include "parameters/scale-converters/Linear100PercentScaleConverter.h"
#include "parameters/scale-converters/Fine12STScaleConverter.h"
#include "parameters/scale-converters/FineBipolar12STScaleConverter.h"
#include <parameters/ModulateableParameterWithUnusualModUnit.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/UnmodulatebaleParameterLayouts.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/ModulateableParameterLayouts.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/ParameterInfoLayout.h>
#include <proxies/hwui/controls/Button.h>
#include <proxies/hwui/HWUI.h>
#include <parameters/scale-converters/dimension/VoicesDimension.h>
#include <parameters/scale-converters/LinearCountScaleConverter.h>
#include <parameters/UnisonVoicesParameter.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/UnisonParameterLayouts.h>

UnisonGroup::UnisonGroup(ParameterDualGroupSet *parent, VoiceGroup vg)
    : ParameterGroup(parent, { "Unison", vg }, "Unison", "Unison", "Unison")
{
}

UnisonGroup::~UnisonGroup() = default;

class UnmodulateableUnisonParameter : public Parameter
{
 public:
  template <typename... tArgs>
  UnmodulateableUnisonParameter(tArgs... args)
      : Parameter(args...)
  {
  }

  DFBLayout *createLayout(FocusAndMode focusAndMode) const override
  {
    switch(focusAndMode.mode)
    {
      case UIMode::Select:
      default:
        return new UnmodulateableUnisonParameterLayout();
      case UIMode::Edit:
        return new UnmodulateableParameterEditLayout2();
      case UIMode::Info:
        return new ParameterInfoLayout();
    }
  }
};

class ModulateableUnisonParameterWithUnusualModUnit : public ModulateableParameterWithUnusualModUnit
{
 public:
  template <typename... tArgs>
  ModulateableUnisonParameterWithUnusualModUnit(tArgs... args)
      : ModulateableParameterWithUnusualModUnit(args...)
  {
  }

  DFBLayout *createLayout(FocusAndMode focusAndMode) const override
  {
    switch(focusAndMode.mode)
    {
      case UIMode::Select:
      default:
        return new ModulateableUnisonParameterLayout();
      case UIMode::Edit:
        return new ModulateableParameterEditLayout2();
      case UIMode::Info:
        return new ParameterInfoLayout();
    }
  }
};

void UnisonGroup::init()
{
  appendParameter(new UnisonVoicesParameter(this, getVoiceGroup()));

  appendParameter(new ModulateableUnisonParameterWithUnusualModUnit(
      this, ParameterId{ 250, getVoiceGroup() }, ScaleConverter::get<Fine12STScaleConverter>(),
      ScaleConverter::get<FineBipolar12STScaleConverter>(), 0, 120, 12000));

  appendParameter(new UnmodulateableUnisonParameter(
      this, ParameterId{ 252, getVoiceGroup() }, ScaleConverter::get<Linear360DegreeScaleConverter>(), 0, 360, 3600));

  appendParameter(new UnmodulateableUnisonParameter(
      this, ParameterId{ 253, getVoiceGroup() }, ScaleConverter::get<Linear100PercentScaleConverter>(), 0, 100, 1000));
}

bool UnisonGroup::isUnisonParameter(const Parameter *parameter)
{
  return isUnisonParameter(parameter->getID());
}

bool UnisonGroup::isUnisonParameter(const ParameterId &id)
{
  const auto n = id.getNumber();
  return n == 249 || n == 250 || n == 252 || n == 253;
}

bool UnisonGroup::isUnisonVoicesParameter(const Parameter *parameter)
{
  return isUnisonVoicesParameter(parameter->getID());
}

bool UnisonGroup::isUnisonVoicesParameter(const ParameterId &id)
{
  return id.getNumber() == 249;
}
