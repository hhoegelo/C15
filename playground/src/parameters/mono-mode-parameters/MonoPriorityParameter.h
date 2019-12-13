#pragma once
#include "MonoParameter.h"

class MonoPriorityParameter : public Parameter
{
 public:
  MonoPriorityParameter(ParameterGroup* parent, ParameterId id);

  Glib::ustring getDisplayString() const override;
  Glib::ustring getLongName() const override;
  Glib::ustring getShortName() const override;
  DFBLayout* createLayout(FocusAndMode focusAndMode) const override;
};
