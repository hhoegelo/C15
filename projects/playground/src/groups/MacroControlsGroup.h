#pragma once

#include "ParameterGroup.h"
#include "parameters/ModulateableParameter.h"

class MacroControlsGroup : public ParameterGroup
{
 public:
  MacroControlsGroup(ParameterGroupSet *parent);
  virtual ~MacroControlsGroup();

  void init();

  static ParameterId modSrcToSmoothingId(MacroControls src);
  static ParameterId modSrcToParamId(MacroControls src);
  static MacroControls paramIDToModSrc(const ParameterId& pid);
  static bool isMacroTime(const ParameterId &id);
  static bool isMacroControl(int paramNumber);
static ParameterId smoothingIdToMCId(const ParameterId& smoothingId);
};
