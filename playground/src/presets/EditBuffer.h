#pragma once

#include "ParameterDualGroupSet.h"
#include "presets/recall/RecallParameterGroups.h"
#include <nltools/threading/Expiration.h>
#include <tools/DelayedJob.h>
#include <tools/Uuid.h>
#include <parameters/SplitPointParameter.h>

class Application;
class Writer;
class PresetManager;

class EditBuffer : public ParameterDualGroupSet
{
 private:
  typedef ParameterDualGroupSet super;

 public:
  EditBuffer(PresetManager *parent);
  ~EditBuffer() override;

  Glib::ustring getName() const;
  Glib::ustring getVoiceGroupName(VoiceGroup vg) const;
  size_t getHash() const;
  const Preset *getOrigin() const;
  Parameter *getSelected() const;
  bool isZombie() const;

  void setMacroControlValueFromMCView(ParameterId id, double value, const Glib::ustring &uuid);

  void undoableSelectParameter(const Glib::ustring &id);
  void undoableSelectParameter(ParameterId id);
  void undoableSelectParameter(Parameter *p);
  void undoableSelectParameter(UNDO::Transaction *transaction, Parameter *p);

  void undoableLoad(UNDO::Transaction *transaction, Preset *preset);
  void undoableLoad(Preset *preset);
  void undoableLoadIntoVoiceGroup(Preset *preset, VoiceGroup vg);

  void undoableLoadSelectedPreset();
  void undoableSetLoadedPresetInfo(UNDO::Transaction *transaction, Preset *preset);
  void undoableUpdateLoadedPresetInfo(UNDO::Transaction *transaction);
  void undoableRandomize(UNDO::Transaction *transaction, Initiator initiator);
  void undoableInitSound(UNDO::Transaction *transaction);
  void undoableSetDefaultValues(UNDO::Transaction *transaction, Preset *values);
  void undoableLockAllGroups(UNDO::Transaction *transaction);
  void undoableUnlockAllGroups(UNDO::Transaction *transaction);
  void undoableToggleGroupLock(UNDO::Transaction *transaction, const Glib::ustring &groupId);

  void setName(UNDO::Transaction *transaction, const Glib::ustring &name);
  void setVoiceGroupName(UNDO::Transaction *transaction, const Glib::ustring &name, VoiceGroup vg);

  void writeDocument(Writer &writer, tUpdateID knownRevision) const override;
  Uuid getUUIDOfLastLoadedPreset() const;
  PresetManager *getParent();
  const PresetManager *getParent() const;

  void resetModifiedIndicator(UNDO::Transaction *transaction);
  void resetModifiedIndicator(UNDO::Transaction *transaction, size_t hash);

  void copyFrom(UNDO::Transaction *transaction, const Preset *preset) override;

  //void copyFrom(UNDO::Transaction *transaction, const LayerPreset *preset);
  //void copyFrom(UNDO::Transaction *transaction, const SplitPreset *preset);

  tUpdateID onChange(uint64_t flags = UpdateDocumentContributor::ChangeFlags::Generic) override;

  bool hasLocks(VoiceGroup vg) const;
  bool anyParameterChanged() const;
  void resetOriginIf(const Preset *p);

  // CALLBACKS
  sigc::connection onSelectionChanged(const slot<void, Parameter *, Parameter *> &s);
  sigc::connection onModificationStateChanged(const slot<void, bool> &s);
  sigc::connection onChange(const slot<void> &s);
  sigc::connection onPresetLoaded(const slot<void> &s);
  sigc::connection onLocksChanged(const slot<void> &s);
  sigc::connection onRecallValuesChanged(const slot<void> &s);
  sigc::connection onSoundTypeChanged(slot<void> s);

  bool isModified() const;
  static void sendToLPC();

  //RECALL
  RecallParameterGroups &getRecallParameterSet();
  void initRecallValues(UNDO::Transaction *t);

  SoundType getType() const;

  void undoableConvertToDual(UNDO::Transaction *transaction, SoundType type);
  void undoableConvertToSingle(UNDO::Transaction *transaction, VoiceGroup copyFrom);

  void undoableLoadPresetIntoDualSound(Preset *preset, VoiceGroup target);
  void undoableLoadPresetIntoDualSound(UNDO::Transaction *transaction, Preset *preset, VoiceGroup target);

  const SplitPointParameter *getSplitPoint() const;
  SplitPointParameter *getSplitPoint();

 private:
  Glib::ustring getEditBufferName() const;
  bool anyParameterChanged(VoiceGroup vg) const;
  Parameter *searchForAnyParameterWithLock(VoiceGroup vg) const;
  UNDO::Scope &getUndoScope() override;
  void setParameter(ParameterId id, double cpValue);

  void undoableSelectParameter(UNDO::Transaction *transaction, const ParameterId &id);
  void undoableSetType(UNDO::Transaction *transaction, SoundType type);
  void undoableConvertToSplit(UNDO::Transaction *transaction);
  void undoableConvertToLayer(UNDO::Transaction *transaction);
  void undoableConvertDualToSingle(UNDO::Transaction *transaction, VoiceGroup copyFrom);

  void setModulationSource(MacroControls src);
  void setModulationAmount(double amount);

  void doDeferedJobs();
  void checkModified();

  Signal<void, Parameter *, Parameter *> m_signalSelectedParameter;
  SignalWithCache<void, bool> m_signalModificationState;
  Signal<void> m_signalChange;
  Signal<void> m_signalPresetLoaded;
  Signal<void> m_signalLocksChanged;
  Signal<void> m_signalTypeChanged;

  ParameterId m_lastSelectedParameter;

  friend class EditBufferSerializer;
  friend class RecallEditBufferSerializer;
  friend class RecallEditBufferSerializer2;
  friend class EditBufferActions;
  friend class PresetParameterGroups;

  Uuid m_lastLoadedPreset;

  Glib::ustring m_name;
  std::array<Glib::ustring, 2> m_voiceGroupLabels;

  DelayedJob m_deferredJobs;

  bool m_isModified;
  SoundType m_type;
  size_t m_hashOnStore;

  mutable Preset *m_originCache{ nullptr };
  RecallParameterGroups m_recallSet;

  friend class PresetManager;
  friend class LastLoadedPresetInfoSerializer;
};
