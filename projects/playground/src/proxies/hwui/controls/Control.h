#pragma once

#include "playground.h"
#include "Rect.h"
#include <nltools/Uncopyable.h>
#include <sigc++/trackable.h>

class Font;
class FrameBuffer;

class Control : public sigc::trackable, public Uncopyable
{
 public:
  Control(const Rect &pos);
  virtual ~Control();

  virtual bool redraw(FrameBuffer &fb);
  virtual void drawBackground(FrameBuffer &fb);

  virtual const Rect &getPosition() const;

  virtual void setHighlight(bool isHighlight);
  bool isHighlight() const;

  void setDirty();
  virtual bool isVisible() const;
  void setVisible(bool b);
  int getHeight() const;
  int getWidth() const;
  virtual void setPosition(const Rect &rect);

 protected:
  virtual void setBackgroundColor(FrameBuffer &fb) const;

 private:
  Rect m_rect;
  bool m_highlight;
  bool m_visible;
};
