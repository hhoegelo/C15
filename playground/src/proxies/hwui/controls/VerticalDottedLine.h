#pragma once

#include "Control.h"

class VerticalDottedLine : public Control
{
 private:
  typedef Control super;

 public:
  VerticalDottedLine(const Rect& rect);
  bool redraw(FrameBuffer& fb);
};
