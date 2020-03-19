#pragma once

#include "PresetLabel.h"
#include <nltools/Types.h>
#include <presets/Preset.h>

class SinglePresetTypeLabel : public PresetLabel
{
 public:
  explicit SinglePresetTypeLabel(const Rect& r);
  void drawBackground(FrameBuffer& fb) override;

  void update(const Preset* newPreset);
};

class DualPresetTypeLabel : public Control
{
 public:
  explicit DualPresetTypeLabel(const Rect& r);
  bool redraw(FrameBuffer& fb) override;
  bool drawLayer(FrameBuffer& buffer);
  bool drawSplit(FrameBuffer& buffer);
  void update(const Preset* selected);

 private:
  bool m_anyLoaded = false;
  bool m_inidicateI = true;
  SoundType m_presetType = SoundType::Single;
};

class PresetTypeLabel : public Control
{
 public:
  explicit PresetTypeLabel(const Rect& pos);
  void update(const Preset* newSelection);

  bool redraw(FrameBuffer& fb) override;

  std::unique_ptr<Control> m_currentControl;
};
