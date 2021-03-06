#pragma once

#include "playground.h"
#include <proxies/hwui/HWUIEnums.h>
#include <proxies/hwui/buttons.h>
#include <nltools/Uncopyable.h>
#include <map>
#include <set>
#include <sigc++/connection.h>

class Application;

class UsageMode : public Uncopyable
{
 public:
  UsageMode();
  virtual ~UsageMode();

  virtual bool onButtonPressed(Buttons buttonID, ButtonModifiers modifiers, bool state);

  virtual void setup() = 0;
  virtual void setupFocusAndMode(FocusAndMode focusAndMode);
  virtual void bruteForceUpdateLeds();

 protected:
  typedef std::function<bool(Buttons button, ButtonModifiers modifiers, bool state)> tAction;

  void setupButtonConnection(Buttons buttonID, tAction action);
  void removeButtonConnection(Buttons buttonID);

#if _TESTS
  std::set<gint32> assignedAudioIDs;
#endif

 private:
  void connectToVoiceGroupSignal();

  static const int num_buttons = 128;

  std::map<Buttons, tAction> m_actions;

  sigc::connection m_voiceGroupChangedSignal;
};
