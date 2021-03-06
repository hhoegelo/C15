#pragma once

#include "BooleanSetting.h"

class SendPresetAsLPCWriteFallback : public BooleanSetting
{
 private:
  typedef BooleanSetting super;

 public:
  SendPresetAsLPCWriteFallback(UpdateDocumentContributor& settings);
  virtual ~SendPresetAsLPCWriteFallback();

 private:
  SendPresetAsLPCWriteFallback(const SendPresetAsLPCWriteFallback& other) = delete;
  SendPresetAsLPCWriteFallback& operator=(const SendPresetAsLPCWriteFallback&) = delete;
};
