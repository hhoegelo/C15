#pragma once

#include <proxies/hwui/controls/Label.h>

class Application;

class BankNameHeadlineLabel : public Label
{
 private:
  typedef Label super;

 public:
  BankNameHeadlineLabel(const Rect &pos);

  virtual bool redraw(FrameBuffer &fb) override;
  virtual void setFontColor(FrameBuffer &fb) const override;
  virtual std::shared_ptr<Font> getFont() const override;
  virtual int getFontHeight() const override;
};
