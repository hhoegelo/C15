#pragma once

#include <tools/StateMachine.h>
#include <ParameterId.h>

enum class MacroControlAssignmentEvents
{
  MCPressedWhileSelected,
  MCPressedWhileUnselected,
  MCReleased,
  ModParamPressed,
  ModParamReleased
};

enum class MacroControlAssignmentStates
{
  Initial,
  Selected,
  Armed,
  Assign,
  SelectSource
};

class MacroControlAssignmentStateMachine
    : public StateMachine<MacroControlAssignmentStates, MacroControlAssignmentEvents,
                          MacroControlAssignmentStates::Initial, MacroControlAssignmentStates::Initial>
{
 private:
  using super = StateMachine<MacroControlAssignmentStates, MacroControlAssignmentEvents,
                             MacroControlAssignmentStates::Initial, MacroControlAssignmentStates::Initial>;

 public:
  MacroControlAssignmentStateMachine();
  virtual ~MacroControlAssignmentStateMachine();

  void setCurrentMCParameter(int mcParamId);
  int getCurrentMCParameter() const;

  void setCurrentModulateableParameter(const ParameterId& id);
  const ParameterId& getCurrentModulateableParameter() const;

 private:
  int m_mcParamId = 0;
  ParameterId m_modParamId;
};
