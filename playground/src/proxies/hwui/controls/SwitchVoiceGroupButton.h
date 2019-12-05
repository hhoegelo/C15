#pragma once

#include "Button.h"
#include <nltools/Types.h>

class SwitchVoiceGroupButton : public Button
{
 public:
  explicit SwitchVoiceGroupButton(Buttons pos);

 private:
  static Glib::ustring getTextFor(VoiceGroup vg);

  void update(VoiceGroup newVoiceGroup);
};