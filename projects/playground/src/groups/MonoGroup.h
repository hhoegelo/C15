#pragma once

#include "ParameterGroup.h"

class MonoGroup : public ParameterGroup
{
 public:
  MonoGroup(ParameterGroupSet* parent, VoiceGroup voicegroup);
  void init();

  static bool isMonoParameter(const ParameterId& id);
  static bool isMonoParameter(const Parameter* param);
};