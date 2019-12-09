#pragma once

#include <proxies/hwui/DFBLayout.h>
#include <proxies/hwui/OLEDProxy.h>

class TestLayout : public DFBLayout
{
 private:
  typedef DFBLayout super;

 public:
  TestLayout(OLEDProxy &parent);
  virtual ~TestLayout();

  void iterate();

 private:
  virtual bool redrawLayout() override;

  enum Tests
  {
    Full = 0,
    Gradient = 1,
    Border = 2,
    NumTests = 3
  };

  int m_test = Full;
  bool m_isDirty = true;
};