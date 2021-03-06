#pragma once

#include <proxies/hwui/controls/Label.h>

class TextEditUsageMode;

class TextEditControlLetter : public Label
{
 private:
  typedef Label super;

 public:
  TextEditControlLetter(std::shared_ptr<TextEditUsageMode> textUsageMode, int relativeToCursor, const Rect &pos);

  virtual bool redraw(FrameBuffer &fb);
  StringAndSuffix getText() const override;

 private:
  std::shared_ptr<TextEditUsageMode> m_textUsageMode;
  int m_relativeToCursor;
};
