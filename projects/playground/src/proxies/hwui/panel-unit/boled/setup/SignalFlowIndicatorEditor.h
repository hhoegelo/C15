#pragma once

#include "MenuEditor.h"

class Setting;

class SignalFlowIndicatorEditor : public MenuEditor
{
 private:
  typedef MenuEditor super;

 public:
  SignalFlowIndicatorEditor();
  virtual ~SignalFlowIndicatorEditor() = default;

  void incSetting(int inc) override;
  const std::vector<Glib::ustring> &getDisplayStrings() const override;
  int getSelectedIndex() const override;
};
