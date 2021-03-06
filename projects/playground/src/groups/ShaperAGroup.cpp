#include "ShaperAGroup.h"
#include "parameters/Parameter.h"
#include "parameters/ModulateableParameter.h"

#include "parameters/scale-converters/Linear100PercentScaleConverter.h"
#include "parameters/scale-converters/LinearBipolar100PercentScaleConverter.h"
#include "parameters/scale-converters/Linear50DbScaleConverter.h"
#include "parameters/scale-converters/LinearBipolar50DbScaleConverter.h"
#include <parameters/scale-converters/LinearBipolar200PercentScaleConverter.h>
#include <parameters/ModulateableParameterWithUnusualModUnit.h>

ShaperAGroup::ShaperAGroup(ParameterDualGroupSet *parent, VoiceGroup vg)
    : ParameterGroup(parent, { "Sh A", vg }, "Shaper A", "Shaper A", "Shaper A")
{
}

ShaperAGroup::~ShaperAGroup()
{
}

void ShaperAGroup::init()
{
  appendParameter(new ModulateableParameterWithUnusualModUnit(
      this, { 71, getVoiceGroup() }, ScaleConverter::get<Linear50DbScaleConverter>(),
      ScaleConverter::get<LinearBipolar50DbScaleConverter>(), 0.2, 100, 500));
  appendParameter(new Parameter(this, { 73, getVoiceGroup() }, ScaleConverter::get<Linear100PercentScaleConverter>(), 0,
                                100, 1000));
  appendParameter(new Parameter(this, { 74, getVoiceGroup() }, ScaleConverter::get<Linear100PercentScaleConverter>(),
                                0.5, 100, 1000));
  appendParameter(new Parameter(this, { 75, getVoiceGroup() }, ScaleConverter::get<Linear100PercentScaleConverter>(), 0,
                                100, 1000));
  appendParameter(new ModulateableParameterWithUnusualModUnit(
      this, { 76, getVoiceGroup() }, ScaleConverter::get<LinearBipolar100PercentScaleConverter>(),
      ScaleConverter::get<LinearBipolar200PercentScaleConverter>(), 0, 100, 1000));
  appendParameter(new ModulateableParameter(this, { 78, getVoiceGroup() },
                                            ScaleConverter::get<Linear100PercentScaleConverter>(), 0, 100, 1000));
  appendParameter(new Parameter(this, { 80, getVoiceGroup() }, ScaleConverter::get<Linear100PercentScaleConverter>(), 0,
                                100, 1000));
  appendParameter(new ModulateableParameter(this, { 81, getVoiceGroup() },
                                            ScaleConverter::get<Linear100PercentScaleConverter>(), 0, 100, 1000));
}
