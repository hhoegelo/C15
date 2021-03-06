#pragma once

#include <proxies/hwui/controls/Label.h>

class PresetNameHeadlineLabel : public Label
{
 private:
  typedef Label super;

 public:
  PresetNameHeadlineLabel(const Rect &pos);

  virtual bool redraw(FrameBuffer &fb) override;
  virtual void setFontColor(FrameBuffer &fb) const override;
  virtual std::shared_ptr<Font> getFont() const override;
  virtual int getFontHeight() const override;
};
