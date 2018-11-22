#include <Application.h>
#include <presets/PresetBank.h>
#include <presets/PresetManager.h>
#include <proxies/hwui/buttons.h>
#include <proxies/hwui/HWUI.h>
#include <proxies/hwui/panel-unit/boled/preset-screens/controls/PresetListContent.h>
#include <proxies/hwui/panel-unit/boled/preset-screens/controls/PresetListHeader.h>
#include <proxies/hwui/panel-unit/boled/preset-screens/controls/PresetListSelectStorePosition.h>
#include <presets/StoreModeData.h>
#include <algorithm>

PresetListSelectStorePosition::PresetListSelectStorePosition(const Rect &pos, bool showBankArrows, StoreModeData *pod)
    : super(pos, showBankArrows)
    , m_storeModeData(pod)
{
  initBankAndPreset();
}

PresetListSelectStorePosition::~PresetListSelectStorePosition()
{
}

void PresetListSelectStorePosition::initBankAndPreset()
{
  onChange();
}

void PresetListSelectStorePosition::onChange()
{
  m_bankConnection.disconnect();

  if(auto bank = Application::get().getPresetManager()->getBank(m_storeModeData->bankPos))
  {
    m_bankConnection = bank->onBankChanged(mem_fun(this, &PresetListSelectStorePosition::onBankChanged));
  }
}

void PresetListSelectStorePosition::onBankChanged()
{
  if(auto bank = Application::get().getPresetManager()->getBank(m_storeModeData->bankPos))
  {
    if(m_selectedPreset != bank->getSelectedPreset())
    {
      m_selectedPreset = bank->getSelectedPreset();
      m_storeModeData->presetPos = static_cast<int>(bank->getPresetPosition(m_selectedPreset));
    }

    sanitizePresetPosition(bank);
    m_content->setup(bank, m_storeModeData->presetPos);
    m_header->setup(bank);
  }
}

std::pair<int, int> PresetListSelectStorePosition::getSelectedPosition() const
{
  return { m_storeModeData->bankPos, m_storeModeData->presetPos };
}

bool PresetListSelectStorePosition::onButton(int i, bool down, ButtonModifiers modifiers)
{
  auto focusAndMode = Application::get().getHWUI()->getFocusAndMode();

  if(down)
  {
    switch(i)
    {
      case BUTTON_B:
        if(focusAndMode.focus == UIFocus::Banks)
        {
          movePresetSelection(-1);
        }
        else
        {
          moveBankSelection(-1);
        }
        return true;

      case BUTTON_C:
        if(focusAndMode.focus == UIFocus::Banks)
        {
          movePresetSelection(1);
        }
        else
        {
          moveBankSelection(1);
        }
        return true;
    }
  }
  return false;
}

void PresetListSelectStorePosition::onRotary(int inc, ButtonModifiers modifiers)
{
  auto focusAndMode = Application::get().getHWUI()->getFocusAndMode();

  if(focusAndMode.focus == UIFocus::Presets)
    movePresetSelection(inc);
  else
    moveBankSelection(inc);
}

void PresetListSelectStorePosition::movePresetSelection(int moveBy)
{
  auto pm = Application::get().getPresetManager();

  sanitizeBankPosition(pm);

  if(auto bank = pm->getBank(m_storeModeData->bankPos))
  {
    m_storeModeData->presetPos += moveBy;
    sanitizePresetPosition(bank);
  }

  onChange();
  setDirty();
}

void PresetListSelectStorePosition::sanitizeBankPosition(std::shared_ptr<PresetManager> pm)
{
  if(auto numBanks = pm->getNumBanks())
  {
    m_storeModeData->bankPos = std::min<int>(m_storeModeData->bankPos, numBanks - 1);
    m_storeModeData->bankPos = std::max<int>(m_storeModeData->bankPos, 0);
    return;
  }

  m_storeModeData->bankPos = invalidIndex;
}

void PresetListSelectStorePosition::sanitizePresetPosition(std::shared_ptr<PresetBank> bank)
{
  if(bank)
  {
    if(auto numPresets = bank->getNumPresets())
    {
      m_storeModeData->presetPos = std::min<int>(m_storeModeData->presetPos, numPresets - 1);
      m_storeModeData->presetPos = std::max<int>(m_storeModeData->presetPos, 0);
      return;
    }
  }

  m_storeModeData->presetPos = invalidIndex;
}

void PresetListSelectStorePosition::moveBankSelection(int moveBy)
{
  auto pm = Application::get().getPresetManager();
  m_storeModeData->bankPos += moveBy;

  sanitizeBankPosition(pm);
  sanitizePresetPosition(pm->getBank(m_storeModeData->bankPos));

  onChange();
  setDirty();
}
