#include <testing/TestHelper.h>
#include <Application.h>
#include <presets/Preset.h>
#include <device-settings/Settings.h>
#include <device-settings/DirectLoadSetting.h>
#include <testing/unit-tests/mock/MockPresetStorage.h>
#include <proxies/hwui/buttons.h>
#include <proxies/hwui/HWUI.h>

namespace detail
{
  auto getDirectLoad()
  {
    return Application::get().getSettings()->getSetting<DirectLoadSetting>();
  }

  auto pressButton(Buttons b)
  {
    auto hwui = Application::get().getHWUI();
    hwui->getPanelUnit().getEditPanel().onButtonPressed(b, {}, true);
    hwui->getPanelUnit().getEditPanel().onButtonPressed(b, {}, false);
  }
}

TEST_CASE("Part Origin Attribute")
{
  auto eb = TestHelper::getEditBuffer();
  auto hwui = Application::get().getHWUI();

  MockPresetStorage presets;

  detail::getDirectLoad()->set(BooleanSettings::BOOLEAN_SETTING_FALSE);
  hwui->setFocusAndMode(UIDetail::Init);

  SECTION("Load Single Full")
  {
    eb->undoableLoad(presets.getSinglePreset());

    auto originI = eb->getPartOrigin(VoiceGroup::I);
    auto originII = eb->getPartOrigin(VoiceGroup::II);

    CHECK(originI.presetUUID == presets.getSinglePreset()->getUuid());
    CHECK(originI.sourceGroup == VoiceGroup::I);
    CHECK(originII.presetUUID == presets.getSinglePreset()->getUuid());
    CHECK(originII.sourceGroup == VoiceGroup::I);
  }

  SECTION("Init Sound Resets")
  {
    auto scope = TestHelper::createTestScope();
    eb->undoableLoad(scope->getTransaction(), presets.getSinglePreset());

    eb->undoableInitSound(scope->getTransaction());
    auto originI = eb->getPartOrigin(VoiceGroup::I);
    auto originII = eb->getPartOrigin(VoiceGroup::II);

    CHECK(originI.presetUUID == "");
    CHECK(originII.presetUUID == "");
    CHECK(originI.sourceGroup == VoiceGroup::Global);
    CHECK(originII.sourceGroup == VoiceGroup::Global);
  }

  SECTION("Init Part Resets")
  {
    auto scope = TestHelper::createTestScope();
    eb->undoableLoad(scope->getTransaction(), presets.getSinglePreset());

    eb->undoableInitPart(scope->getTransaction(), VoiceGroup::I);
    auto originI = eb->getPartOrigin(VoiceGroup::I);
    CHECK(originI.presetUUID == "");
    CHECK(originI.sourceGroup == VoiceGroup::Global);

    eb->undoableInitPart(scope->getTransaction(), VoiceGroup::II);
    auto originII = eb->getPartOrigin(VoiceGroup::II);
    CHECK(originII.presetUUID == "");
    CHECK(originII.sourceGroup == VoiceGroup::Global);
  }

  SECTION("Load Dual Full")
  {
    eb->undoableLoad(presets.getLayerPreset());
    auto originI = eb->getPartOrigin(VoiceGroup::I);
    auto originII = eb->getPartOrigin(VoiceGroup::II);

    CHECK(originI.presetUUID == presets.getLayerPreset()->getUuid());
    CHECK(originII.presetUUID == presets.getLayerPreset()->getUuid());
    CHECK(originI.sourceGroup == VoiceGroup::I);
    CHECK(originII.sourceGroup == VoiceGroup::II);
  }

  SECTION("Load Part Of Dual")
  {
    TestHelper::initDualEditBuffer<SoundType::Layer>();
    auto scope = TestHelper::createTestScope();

    eb->undoableLoadToPart(scope->getTransaction(), presets.getSplitPreset(), VoiceGroup::I, VoiceGroup::I);

    auto originI = eb->getPartOrigin(VoiceGroup::I);
    CHECK(originI.presetUUID == presets.getSplitPreset()->getUuid());
    CHECK(originI.sourceGroup == VoiceGroup::I);

    eb->undoableLoadToPart(scope->getTransaction(), presets.getSplitPreset(), VoiceGroup::II, VoiceGroup::II);

    auto originII = eb->getPartOrigin(VoiceGroup::II);
    CHECK(originII.presetUUID == presets.getSplitPreset()->getUuid());
    CHECK(originII.sourceGroup == VoiceGroup::II);
  }
}

auto getOrigins()
{
  auto eb = TestHelper::getEditBuffer();
  return std::pair { eb->getPartOrigin(VoiceGroup::I), eb->getPartOrigin(VoiceGroup::II) };
};

auto forceAutoLoadIfPending()
{
  auto pm = Application::get().getPresetManager();

  if(pm->isAutoLoadScheduled())
    pm->TEST_forceScheduledAutoLoad();
}

TEST_CASE("Step Direct Load and Load to Part Preset List", "[Preset][Loading]")
{
  DualPresetBank presets;
  auto bank = presets.getBank();
  auto hwui = Application::get().getHWUI();

  TestHelper::initDualEditBuffer<SoundType::Layer>();

  auto eb = TestHelper::getEditBuffer();
  auto pm = Application::get().getPresetManager();

  Application::get().getHWUI()->setCurrentVoiceGroup(VoiceGroup::I);
  auto currentVG = Application::get().getHWUI()->getCurrentVoiceGroup();

  CHECK(currentVG == VoiceGroup::I);

  auto [ogI, ogII] = getOrigins();

  CHECK(eb->getOrigin() == nullptr);
  CHECK(ogI.presetUUID == "");
  CHECK(ogII.presetUUID == "");
  CHECK(eb->getUUIDOfLastLoadedPreset() == Uuid::init());

  SECTION("Select First Preset in Bank")
  {

    detail::getDirectLoad()->set(BooleanSettings::BOOLEAN_SETTING_TRUE);

    {
      auto scope = TestHelper::createTestScope();
      auto transaction = scope->getTransaction();
      pm->selectBank(transaction, bank->getUuid());
      bank->selectPreset(transaction, 0);

      Application::get().getHWUI()->undoableSetFocusAndMode(transaction,
                                                            { UIFocus::Presets, UIMode::Select, UIDetail::Init });
    }

    forceAutoLoadIfPending();

    auto uuid = eb->getUUIDOfLastLoadedPreset();
    CHECK(uuid == bank->getPresetAt(0)->getUuid());
    hwui->setFocusAndMode(UIDetail::LoadToPart);

    detail::pressButton(Buttons::BUTTON_INC);
    forceAutoLoadIfPending();
    {
      auto [ogI, ogII] = getOrigins();
      CHECK(ogI.presetUUID == bank->getPresetAt(0)->getUuid());
      CHECK(ogI.sourceGroup == VoiceGroup::II);
    }

    detail::pressButton(Buttons::BUTTON_INC);
    forceAutoLoadIfPending();
    {
      auto [ogI, ogII] = getOrigins();
      CHECK(ogI.presetUUID == bank->getPresetAt(1)->getUuid());
      CHECK(ogI.sourceGroup == VoiceGroup::I);
    }

    detail::pressButton(Buttons::BUTTON_INC);
    forceAutoLoadIfPending();
    {
      auto [ogI, ogII] = getOrigins();
      CHECK(ogI.presetUUID == bank->getPresetAt(1)->getUuid());
      CHECK(ogI.sourceGroup == VoiceGroup::II);
    }

    detail::pressButton(Buttons::BUTTON_DEC);
    forceAutoLoadIfPending();
    {
      auto [ogI, ogII] = getOrigins();
      CHECK(ogI.presetUUID == bank->getPresetAt(1)->getUuid());
      CHECK(ogI.sourceGroup == VoiceGroup::I);
    }

    detail::pressButton(Buttons::BUTTON_DEC);
    forceAutoLoadIfPending();
    {
      auto [ogI, ogII] = getOrigins();
      CHECK(ogI.presetUUID == bank->getPresetAt(0)->getUuid());
      CHECK(ogI.sourceGroup == VoiceGroup::II);
    }

    detail::pressButton(Buttons::BUTTON_DEC);
    forceAutoLoadIfPending();
    {
      auto [ogI, ogII] = getOrigins();
      CHECK(ogI.presetUUID == bank->getPresetAt(0)->getUuid());
      CHECK(ogI.sourceGroup == VoiceGroup::I);
    }
  }
}