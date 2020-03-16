#include <testing/TestHelper.h>
#include <proxies/hwui/HWUI.h>
#include <proxies/hwui/panel-unit/boled/parameter-screens/ParameterLayout.h>
#include <proxies/hwui/panel-unit/boled/preset-screens/PresetManagerLayout.h>

TEST_CASE("Test Previous Boled Focus on Button")
{
  auto hwui = Application::get().getHWUI();
  auto& editPanel = hwui->getPanelUnit().getEditPanel();

  hwui->setFocusAndMode({ UIFocus::Parameters, UIMode::Select, UIDetail::Init });

  {
    auto scope = TestHelper::createTestScope();
    TestHelper::getEditBuffer()->undoableSelectParameter(scope->getTransaction(), { 0, VoiceGroup::I });
  }

  editPanel.getBoled().bruteForce();

  auto getLayout = [hwui, &editPanel] { return editPanel.getBoled().getLayout().get(); };

  auto isParameterLayout = [](auto layout) { return dynamic_cast<const ParameterLayout2*>(layout) != nullptr; };

  auto isPresetLayout = [](auto layout) { return dynamic_cast<const PresetManagerLayout*>(layout) != nullptr; };

  auto pressButton = [&editPanel](Buttons button) {
    editPanel.onButtonPressed(button, {}, true);
    editPanel.onButtonPressed(button, {}, false);
  };

  auto getFocusAndMode = [&hwui] { return hwui->getFocusAndMode(); };

  CHECK(isParameterLayout(getLayout()));

  SECTION("Parameter to Preset and Back") {
    auto oldFocus = getFocusAndMode();
    pressButton(Buttons::BUTTON_PRESET);
    CHECK(isPresetLayout(getLayout()));
    pressButton(Buttons::BUTTON_PRESET);
    CHECK(getFocusAndMode() == oldFocus);
  }

  SECTION("Parameter to Store and Back") {
    auto oldFocus = getFocusAndMode();
    pressButton(Buttons::BUTTON_STORE);
    CHECK(isPresetLayout(getLayout()));
    pressButton(Buttons::BUTTON_STORE);
    auto newFocus = getFocusAndMode();
    CHECK(newFocus == oldFocus);
  }
}