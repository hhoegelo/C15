#pragma once
#include <libundo/undo/TransactionCreationScope.h>
#include <memory>
#include <presets/PresetManager.h>
#include <presets/EditBuffer.h>
#include <parameters/Parameter.h>
#include <catch.hpp>
#include <libundo/undo/Scope.h>
#include <Application.h>
#include <parameters/ModulateableParameter.h>

namespace TestHelper
{
  namespace floating
  {
    template <typename T> inline bool equals(T first, T second)
    {
      constexpr auto epsilon = std::numeric_limits<T>::epsilon();
      return std::abs(first - second) <= epsilon;
    }

    template <typename T> inline bool differs(T first, T second)
    {
      return !equals(first, second);
    }
  }

  inline PresetManager* getPresetManager()
  {
    return Application::get().getPresetManager();
  }

  inline EditBuffer* getEditBuffer()
  {
    return getPresetManager()->getEditBuffer();
  }

  inline std::unique_ptr<UNDO::TransactionCreationScope> createTestScope()
  {
    return std::move(getPresetManager()->getUndoScope().startTestTransaction());
  }

  template <SoundType tType> inline void initDualEditBuffer(UNDO::Transaction* transaction)
  {
    auto eb = getEditBuffer();
    eb->undoableUnlockAllGroups(transaction);
    eb->undoableConvertToDual(transaction, tType);
    eb->undoableInitSound(transaction);
  }

  template <SoundType tType> inline void initDualEditBuffer()
  {
    auto scope = UNDO::Scope::startTrashTransaction();
    initDualEditBuffer<tType>(scope->getTransaction());
  }

  inline void initSingleEditBuffer(UNDO::Transaction* transaction)
  {
    auto eb = getEditBuffer();
    eb->undoableUnlockAllGroups(transaction);
    eb->undoableConvertToSingle(transaction, VoiceGroup::I);
    eb->undoableInitSound(transaction);
  }

  inline void forceParameterChange(UNDO::Transaction* transaction, Parameter* param)
  {
    auto currentValue = param->getControlPositionValue();

    auto incNext = param->getValue().getNextStepValue(1, {});
    auto decNext = param->getValue().getNextStepValue(-1, {});

    if(floating::differs(incNext, currentValue))
      param->setCPFromHwui(transaction, incNext);
    else if(floating::differs(decNext, currentValue))
      param->setCPFromHwui(transaction, decNext);
    else
      nltools_detailedAssertAlways(false, "Unable to change Parameter Value in either direction");
  }

  template <typename tCB> inline void forEachParameter(const tCB& cb, EditBuffer* eb)
  {
    for(auto vg : { VoiceGroup::I, VoiceGroup::II, VoiceGroup::Global })
      for(auto& g : eb->getParameterGroups(vg))
        for(auto& p : g->getParameters())
          cb(p);
  }

  inline void changeAllParameters(UNDO::Transaction* transaction)
  {
    auto eb = TestHelper::getEditBuffer();
    eb->forEachParameter([&](Parameter* param) { TestHelper::forceParameterChange(transaction, param); });
  };

  void randomizeCrossFBAndToFX(UNDO::Transaction* transaction);

  void randomizeFadeParams(UNDO::Transaction* transaction);
}

inline std::pair<double, double> getNextStepValuesFromValue(Parameter* p, double v)
{
  auto scope = UNDO::Scope::startTrashTransaction();
  auto oldCP = p->getControlPositionValue();
  p->setCPFromHwui(scope->getTransaction(), v);
  auto ret
      = std::make_pair<double, double>(p->getValue().getNextStepValue(-1, {}), p->getValue().getNextStepValue(1, {}));
  p->setCPFromHwui(scope->getTransaction(), oldCP);
  return ret;
}

#define CHECK_PARAMETER_CP_EQUALS_FICTION(p, v)                                                                        \
  {                                                                                                                    \
    auto range = getNextStepValuesFromValue(p, v);                                                                     \
    auto inRange = p->getControlPositionValue() >= range.first && p->getControlPositionValue() <= range.second;        \
    if(!inRange)                                                                                                       \
    {                                                                                                                  \
      nltools::Log::error(p->getLongName(), '(', p->getID().toString(), ") got", p->getControlPositionValue(),         \
                          "expected ~", v);                                                                            \
      CHECK(inRange);                                                                                                  \
      return;                                                                                                          \
    }                                                                                                                  \
    CHECK(p->getControlPositionValue() >= range.first);                                                                \
    CHECK(p->getControlPositionValue() <= range.second);                                                               \
  }

#define CHECK_PARAMETER_CP_UNEQUALS_FICTION(p, v)                                                                      \
  {                                                                                                                    \
    auto range = getNextStepValuesFromValue(p, v);                                                                     \
    auto inRange = p->getControlPositionValue() >= range.first && p->getControlPositionValue() <= range.second;        \
    if(inRange)                                                                                                        \
    {                                                                                                                  \
      nltools::Log::error(p->getLongName(), '(', p->getID().toString(), ") got", p->getControlPositionValue(),         \
                          "expected unequals ~", v);                                                                   \
      CHECK(false);                                                                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    CHECK_FALSE(p->getControlPositionValue() >= range.first);                                                          \
    CHECK_FALSE(p->getControlPositionValue() <= range.second);                                                         \
  }
