#pragma once

#include "EditBufferEvent.h"

namespace DescriptiveLayouts
{
  class EditBufferTypeStringEvent : public EditBufferEvent<DisplayString>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class EditBufferName : public EditBufferEvent<DisplayString>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class EditBufferNameWithSuffix : public EditBufferEvent<DisplayString>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class CurrentVoiceGroupLabel : public EditBufferEvent<DisplayString>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class IsCurrentVGI : public EditBufferEvent<bool>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class IsCurrentVGII : public EditBufferEvent<bool>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class VGIMuted : public EditBufferEvent<DisplayString>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class VGIIMuted : public EditBufferEvent<DisplayString>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class IsSingleSound : public EditBufferEvent<bool>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class IsLayerSound : public EditBufferEvent<bool>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class IsSplitSound : public EditBufferEvent<bool>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class CurrentVoiceGroupText : public VoiceGroupSelectedEvent<DisplayString>
  {
   public:
    void onChange(VoiceGroup newSelection) override;
  };

  class SoundParamsButtonText : public EditBufferEvent<DisplayString>
  {
   public:
    void onChange(const EditBuffer *eb) override;

   private:
    bool isChanged(const EditBuffer *eb);
  };

  class SoundVoicesButtonText : public EditBufferEvent<DisplayString>
  {
   public:
    void onChange(const EditBuffer *eb) override;

   private:
    bool isChanged(const EditBuffer *eb);
  };

  class SoundMasterButtonText : public EditBufferEvent<DisplayString>
  {
   public:
    void onChange(const EditBuffer *eb) override;

   private:
    bool isChanged(const EditBuffer *eb);
  };

  class MonoButtonText : public EditBufferEvent<DisplayString>
  {
   public:
    void onChange(const EditBuffer *eb) override;

   private:
    bool isChanged(const EditBuffer *eb);
  };

  class UnisonButtonText : public EditBufferEvent<DisplayString>
  {
   public:
    void onChange(const EditBuffer *eb) override;

   private:
    bool isChanged(const EditBuffer *eb);
  };

  class AnyLayerCrossFB : public EditBufferEvent<bool>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class AnyLayerCrossFBToI : public EditBufferEvent<bool>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class AnyLayerCrossFBToII : public EditBufferEvent<bool>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  //ToFXIIUnder100, ToFXIUnder100, ToFXIIOver0, ToFXIOver0

  class ToFXIUnder100 : public EditBufferEvent<bool>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class ToFXIIUnder100 : public EditBufferEvent<bool>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class ToFXIOver0 : public EditBufferEvent<bool>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };

  class ToFXIIOver0 : public EditBufferEvent<bool>
  {
   public:
    void onChange(const EditBuffer *eb) override;
  };
}
