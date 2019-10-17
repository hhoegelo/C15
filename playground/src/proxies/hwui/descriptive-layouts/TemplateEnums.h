#pragma once

#include <playground.h>
#include <proxies/hwui/FrameBuffer.h>
#include <tools/EnumTools.h>
#include <boost/serialization/strong_typedef.hpp>
#include <proxies/hwui/buttons.h>

namespace DescriptiveLayouts
{
  using ComponentID = Glib::ustring;

  ENUM(PrimitiveProperty, uint8_t, None, Text, Range, Visibility, Highlight, ControlPosition);
  ENUM(StyleKey, uint8_t, BackgroundColor, Color, TextAlign, FontSize, BorderStyle, FontDecoration, SuffixColor,
       BorderColor, HighlightBackgroundColor, HighlightColor);
  ENUM(ParameterType, uint8_t, Unipolar = 1 << 0, Bipolar = 1 << 1);
  ENUM(ButtonEvents, uint8_t, Down = true, Up = false);

  namespace StyleValues
  {
    ENUM(Font, uint8_t, Undefined, Regular, Bold);
    ENUM(Color, int, Undefined = FrameBuffer::Undefined, Transparent = FrameBuffer::Transparent, C43 = FrameBuffer::C43,
         C77 = FrameBuffer::C77, C103 = FrameBuffer::C103, C128 = FrameBuffer::C128, C179 = FrameBuffer::C179,
         C204 = FrameBuffer::C204, C255 = FrameBuffer::C255);
    ENUM(Alignment, uint8_t, Undefined, Left, Center, Right);
    ENUM(BorderStyle, uint8_t, Undefined, Solid, Rounded, None);
  }

  ENUM(EventSources, uint8_t, Any, None, ParameterName, SliderRange, ParameterDisplayString, ParameterGroupName,
       IsBipolar, LockStatus, MacroControlSymbol, MacroControlAssignment, MacroControlAmount, MacroControlPosition,
       MacroControlPositionText, MCModRange, EditBufferTypeText, EditBufferName, CurrentVoiceGroupName,
       ParameterControlPosition, ParameterNameWithStateSuffix, IsNotOnlyParameterOnButton, IsOnlyParameterOnButton,
       BooleanTrue, BooleanFalse, MCSelectionChanged, MCPositionChanged, MCAmountChanged, ParameterValueChanged,
       isCurrentVGI, isCurrentVGII, SelectVGButtonText, SoundEditHeading, DirectLoadStatus, PresetListBankName,
       PresetListPresetName, CanLeft, CanRight, isFineActive, EditBufferMasterText, EditBufferUnisonText, VGIUnisonText,
       VGIIUnisonText, VGIMasterText, VGIIMasterText,

       // try to use more generic names, the specific meaning is implemented in the EventProvider
       Edit, Fine, Locked, Position, NumItems,

       ButtonA, ButtonB, ButtonC, ButtonD,

       PreviousNumber, PreviousName, PreviousSelected, PreviousLoaded,

       CurrentNumber, CurrentName, CurrentSelected, CurrentLoaded,

       NextNumber, NextName, NextSelected, NextLoaded);

  ENUM(EventSinks, uint8_t, SwitchToInitDetail, SwitchToEditMode, SwitchToSelectMode, SwitchToSetupFocus,
       SwitchToParameterFocus, SwitchToBankFocus, SwitchToPresetFocus, SwitchToSoundFocus, SwitchToMCSelectDetail,
       SwitchToButtonADetail, SwitchToButtonBDetail, SwitchToButtonCDetail, SwitchToButtonDDetail,
       SwitchToMCModRangeDetail, SwitchToMCAmtDetail, SelectPresetForVoiceGroup, IncMCSel, DecMCSel, IncMCAmt, DecMCAmt,
       IncMCPos, DecMCPos, IncButtonMenu, DecButtonMenu, FireButtonMenu, IncModulationCarousel, DecModulationCarousel,
       ToggleVoiceGroup, CommitPresetForButtonMenu, ToggleDirectLoad, IncPresetSelectionPresetList,
       DecPresetSelectionPresetList, IncBankSelectionPresetList, DecBankSelectionPresetList, DoPresetListAction,
       OpenUnisonParameter, OpenMasterParameter,

       // try to use more generic names, the specific meaning is implemented in the EventProvider
       Left, Right, Up, Down, IncParam, DecParam);

  ENUM(EventProviders, uint8_t, None, Global, IndependentPresetSelectionEvents, PresetManagerEvents);
  ENUM(PrimitiveClasses, uint8_t, Any, Bar, Border, Text, Circle);

  template <typename Derived> class StringId : public std::string
  {
    using super = std::string;

   public:
    StringId() = default;

    constexpr StringId(const super &i)
        : super(i)
    {
    }

    StringId(const char *i)
        : super(i)
    {
    }

    static const StringId<Derived> None;
    static const StringId<Derived> Any;
  };

  template <typename Derived> const StringId<Derived> StringId<Derived>::Any = "ANY";
  template <typename Derived> const StringId<Derived> StringId<Derived>::None = "NONE";

  namespace traits
  {
    namespace Primitives
    {
      struct Instance
      {
      };

      struct Tag
      {
      };

      struct Text
      {
      };
    }

    namespace Controls
    {
      struct Instance
      {
      };
      struct Class
      {
      };
    }

    namespace Layouts
    {
      struct Class
      {
      };
    }
  }

  using PrimitiveTag = StringId<traits::Primitives::Tag>;
  using PrimitiveInstances = StringId<traits::Primitives::Instance>;
  using ControlClasses = StringId<traits::Controls::Class>;
  using ControlInstances = StringId<traits::Controls::Instance>;
  using LayoutClasses = StringId<traits::Layouts::Class>;

  struct EventSinkMapping
  {
   public:
    EventSinkMapping(Buttons button, EventSinks sink, ButtonEvents event = ButtonEvents::Down, bool rep = false)
        : button(button)
        , sink(sink)
        , event(event)
        , repeat(rep)
    {
    }

    Buttons button;
    ButtonEvents event;
    EventSinks sink;
    bool repeat;
  };
}