#pragma once

#include "ParameterLayout.h"
#include <proxies/hwui/controls/ControlWithChildren.h>

class ModulateableParameterLayout2 : public virtual ParameterLayout2
{
 public:
  typedef ParameterLayout2 super;
  ModulateableParameterLayout2();

  void addModAmountSliders(ControlOwner *o);
  virtual bool switchToNormalMode();
};

class ModulateableParameterSelectLayout2 : public ParameterSelectLayout2, public ModulateableParameterLayout2
{
 public:
  typedef ParameterSelectLayout2 super1;
  typedef ModulateableParameterLayout2 super2;
  ModulateableParameterSelectLayout2();

  virtual void copyFrom(Layout *other) override;
  void installMcAmountScreen();

 protected:
  virtual void init() override;
  virtual bool onButton(int i, bool down, ButtonModifiers modifiers) override;
  virtual bool onRotary(int inc, ButtonModifiers modifiers) override;
  virtual Parameter *getCurrentEditParameter() const override;
  virtual void setDefault() override;
  virtual bool switchToNormalMode() override;

 private:
  void onSelectedParameterChanged(Parameter *oldParam, Parameter *newParam);
  void onCurrentParameterChanged(const Parameter *p);
  void fixModeIfNecessary(const Parameter *p);

  enum class Mode
  {
    ParameterValue,
    MacroControlPosition,
    MacroControlSelection,
    MacroControlAmount,
    CarouselUpperBound,
    CarouselParameterValue,
    CarouselLowerBound
  };

  void setMode(Mode desiredMode);
  void toggleMode(Mode desiredMode);
  bool hasModulationSource() const;

  Mode m_mode = Mode::ParameterValue;
  Overlay *m_modeOverlay = nullptr;
  sigc::connection m_paramConnection;
};

class ModulateableParameterEditLayout2 : public ParameterEditLayout2, public ModulateableParameterLayout2
{
 public:
  typedef ParameterEditLayout2 super1;
  typedef ModulateableParameterLayout2 super2;

  ModulateableParameterEditLayout2();
  virtual ButtonMenu *createMenu(const Rect &rect) override;
};
