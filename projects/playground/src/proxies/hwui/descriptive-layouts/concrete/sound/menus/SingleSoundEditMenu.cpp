#include "SingleSoundEditMenu.h"

#include <proxies/hwui/descriptive-layouts/concrete/sound/menus/items/InitSound.h>
#include <proxies/hwui/descriptive-layouts/concrete/sound/menus/items/RandomizeItem.h>
#include <nltools/Types.h>
#include <proxies/hwui/descriptive-layouts/concrete/sound/menus/items/ConvertToSoundTypeItem.h>

SingleSoundEditMenu::SingleSoundEditMenu(const Rect &r)
    : ScrollMenu(r)
{
  init();
}

void SingleSoundEditMenu::init()
{
  auto fullWidth = Rect{ 0, 0, 256, 13 };
  addItem<ConvertToSoundTypeItem>(fullWidth, SoundType::Split);
  addItem<ConvertToSoundTypeItem>(fullWidth, SoundType::Layer);
  addItem<RandomizeItem>(fullWidth);
}
