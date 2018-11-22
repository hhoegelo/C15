#pragma once

#include "SetupLabel.h"
#include "SetupEditor.h"

class Setting;

class RibbonRelativeFactorSettingEditor : public SetupLabel, public SetupEditor
{
 private:
  typedef SetupLabel super;

 public:
  RibbonRelativeFactorSettingEditor();
  virtual ~RibbonRelativeFactorSettingEditor();

  virtual Font::Justification getJustification() const override;

 private:
  bool redraw(FrameBuffer &fb) override;
  void setBackgroundColor(FrameBuffer &fb) const override;
  void onSettingChanged(const Setting *s);
  bool onButton(int i, bool down, ButtonModifiers modifiers) override;
  bool onRotary(int inc, ButtonModifiers modifiers) override;
  void setFontColor(FrameBuffer &fb) const override;

  SetupLabel *m_label = nullptr;
};
