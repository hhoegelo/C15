#pragma once

#include <io/Bridge.h>

class FromLPCBridge : public Bridge
{
 public:
  FromLPCBridge();
  void sendRibbonPosition(bool m_upperRibon, double value);

 private:
  void transmit(const Receiver::tMessage &msg) override;
};
