#include "UnisonGroup.h"

#include "parameters/Parameter.h"
#include "parameters/ModulateableParameter.h"

#include "parameters/scale-converters/Linear12CountScaleConverter.h"
#include "parameters/scale-converters/Linear360DegreeScaleConverter.h"
#include "parameters/scale-converters/Linear100PercentScaleConverter.h"
#include "parameters/scale-converters/Fine12STScaleConverter.h"
#include "parameters/scale-converters/FineBipolar12STScaleConverter.h"
#include <parameters/ModulateableParameterWithUnusualModUnit.h>

UnisonGroup::UnisonGroup(ParameterDualGroupSet *parent, VoiceGroup voicegroup)
    : ParameterGroup(parent, "Unison", "Unison", "Unison", "Unison", voicegroup)
{
}

UnisonGroup::~UnisonGroup()
{
}

void UnisonGroup::init()
{
  appendParameter(
      new Parameter(this, { 249, getVoiceGroup() }, ScaleConverter::get<Linear12CountScaleConverter>(), 0, 11, 11));

  appendParameter(new ModulateableParameterWithUnusualModUnit(
      this, { 250, getVoiceGroup() }, ScaleConverter::get<Fine12STScaleConverter>(),
      ScaleConverter::get<FineBipolar12STScaleConverter>(), 0, 120, 12000));

  appendParameter(new Parameter(this, { 252, getVoiceGroup() }, ScaleConverter::get<Linear360DegreeScaleConverter>(), 0,
                                360, 3600));

  appendParameter(new Parameter(this, { 253, getVoiceGroup() }, ScaleConverter::get<Linear100PercentScaleConverter>(),
                                0, 100, 1000));
}
