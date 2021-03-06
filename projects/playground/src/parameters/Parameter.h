#pragma once

#include "playground.h"
#include "http/UpdateDocumentContributor.h"
#include "value/QuantizedValue.h"
#include <proxies/hwui/HWUIEnums.h>
#include <tools/FlagOwner.h>
#include <ParameterId.h>
#include <tools/Signal.h>

class Layout;
class Writer;
class ParameterGroup;
class MessageComposer;
class PresetParameter;
class RecallParameter;
class ScaleConverter;

enum ParameterFlags
{
  Loaded = 1
};

namespace UNDO
{
  class Transaction;
}

class Parameter : public UpdateDocumentContributor,
                  public IntrusiveListItem<Parameter *>,
                  public FlagOwner<ParameterFlags, uint8_t>
{
 public:
  enum class Step
  {
    STEP_INC,
    STEP_DEC
  };

  enum class VisualizationStyle
  {
    Bar,
    BarFromRight,
    Dot
  };

  Parameter(ParameterGroup *group, ParameterId id, const ScaleConverter *scaling, tControlPositionValue def,
            tControlPositionValue coarseDenominator, tControlPositionValue fineDenominator);
  virtual ~Parameter();

  tControlPositionValue expropriateSnapshotValue();

  const ParameterGroup *getParentGroup() const;
  ParameterGroup *getParentGroup();
  ParameterId getID() const;

  bool isBiPolar() const;
  tControlPositionValue getDefaultValue() const;

  void setDefaultFromHwui();
  void setDefaultFromHwui(UNDO::Transaction *transaction);
  void setDefaultFromWebUI(UNDO::Transaction *transaction);

  virtual void setCPFromHwui(UNDO::Transaction *transaction, const tControlPositionValue &cpValue);
  virtual void setCPFromWebUI(UNDO::Transaction *transaction, const tControlPositionValue &cpValue);

  void stepCPFromHwui(UNDO::Transaction *transaction, int incs, ButtonModifiers modifiers);
  void stepCPFromWebUI(UNDO::Transaction *transaction, Step step, ButtonModifiers modifiers);
  void setIndirect(UNDO::Transaction *transaction, const tControlPositionValue &value);

  virtual void loadFromPreset(UNDO::Transaction *transaction, const tControlPositionValue &value);

  virtual void loadDefault(UNDO::Transaction *transaction);
  virtual void reset(UNDO::Transaction *transaction, Initiator initiator);
  virtual void copyFrom(UNDO::Transaction *transaction, const PresetParameter *other);
  virtual void copyFrom(UNDO::Transaction *transaction, const Parameter *other);
  virtual void copyTo(UNDO::Transaction *transaction, PresetParameter *other) const;

  virtual bool isDefaultLoaded() const;

  virtual void undoableRandomize(UNDO::Transaction *transaction, Initiator initiator, double amount);

  void undoableLoadValue(UNDO::Transaction *transaction, const Glib::ustring &value);
  void undoableSetDefaultValue(UNDO::Transaction *transaction, const PresetParameter *values);

  void undoableLock(UNDO::Transaction *transaction);
  void undoableUnlock(UNDO::Transaction *transaction);
  virtual bool isLocked() const;

  virtual void exportReaktorParameter(std::stringstream &target) const;

  tTcdValue getTcdValue() const;
  tDisplayValue getDisplayValue() const;
  virtual tControlPositionValue getControlPositionValue() const;
  virtual Glib::ustring getDisplayString() const;

  QuantizedValue &getValue();
  const QuantizedValue &getValue() const;
  virtual size_t getHash() const;

  void writeDocument(Writer &writer, tUpdateID knownRevision) const override;
  virtual void writeDiff(Writer &writer, Parameter *other) const;
  virtual void writeDifferences(Writer &writer, Parameter *other) const;
  virtual void writeToLPC(MessageComposer &cmp) const;
  virtual void onPresetSentToLpc() const;

  void invalidate();

  virtual Glib::ustring getLongName() const;
  virtual Glib::ustring getShortName() const;
  Glib::ustring getInfoText() const;

  virtual Glib::ustring getMiniParameterEditorName() const;
  virtual Glib::ustring getGroupAndParameterName() const;
  virtual Glib::ustring getGroupAndParameterNameWithVoiceGroup() const;
  virtual Layout *createLayout(FocusAndMode focusAndMode) const;

  virtual void onUnselected();
  virtual void onSelected();

  virtual VisualizationStyle getVisualizationStyle() const;
  virtual bool lockingEnabled() const;
  // CALLBACKS
  sigc::connection onParameterChanged(sigc::slot<void, const Parameter *> slot, bool doInitCall = true) const;

  void check();

  //Recall
  void undoableRecallFromPreset();
  void undoableRecallFromPreset(UNDO::Transaction *transaction);
  const RecallParameter *getOriginalParameter() const;

  virtual bool isChangedFromLoaded() const;
  bool isValueDifferentFrom(double d) const;

  VoiceGroup getVoiceGroup() const;

 protected:
  virtual void sendToLpc() const;
  void setCpValue(UNDO::Transaction *transaction, Initiator initiator, tControlPositionValue value, bool doSendToLpc);
  virtual void writeDocProperties(Writer &writer, tUpdateID knownRevision) const;
  virtual void onValueChanged(Initiator initiator, tControlPositionValue oldValue, tControlPositionValue newValue);
  virtual void onValueFineQuantizedChanged(Initiator initiator, tControlPositionValue oldValue,
                                           tControlPositionValue newValue);
  virtual bool shouldWriteDocProperties(tUpdateID knownRevision) const;
  virtual tControlPositionValue getNextStepValue(int incs, ButtonModifiers modifiers) const;
  void undoableSetDefaultValue(UNDO::Transaction *transaction, tControlPositionValue value);

 private:
  mutable Signal<void, const Parameter *> m_signalParamChanged;

  ParameterId m_id;
  QuantizedValue m_value;
  sigc::connection m_valueChangedConnection;
  VoiceGroup m_voiceGroup;

  friend class QuantizedValue;
  friend class EditBufferSnapshotMaker;

  tControlPositionValue m_lastSnapshotedValue;
  bool m_isLocked = false;

  virtual void sendParameterMessage() const;
};
