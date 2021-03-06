#pragma once

#include "proxies/hwui/Layout.h"

class Label;
class Setting;

class PlayOrEditModeLayout : public Layout
{
 private:
  typedef Layout super;

 public:
  PlayOrEditModeLayout();
  virtual ~PlayOrEditModeLayout();

 protected:
  int getBehaviourLeft() const;

 private:
  PlayOrEditModeLayout(const PlayOrEditModeLayout& other);
  PlayOrEditModeLayout& operator=(const PlayOrEditModeLayout&);

  void onLastTouchedRibbonChanged(int lastTouchedRibbonParameterID);

  Label* m_upperArrow = nullptr;
  Label* m_lowerArrow = nullptr;
};
