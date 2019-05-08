#pragma once
#include <libundo/undo/Transaction.h>
#include <http/UpdateDocumentContributor.h>

class RecallParameterGroups;
class Parameter;

class RecallParameter : public UpdateDocumentContributor
{
 public:
  RecallParameter(RecallParameterGroups* parent, const int id);
  void writeDocument(Writer& writer, tUpdateID knownRevision) const override;
  void copyFrom(UNDO::Transaction* transaction, const Parameter* other);

  tControlPositionValue getRecallValue() const;
  ModulationSource getRecallModSource() const;
  tControlPositionValue getRecallModulationAmount() const;

 private:
  int m_id;
  tControlPositionValue m_recallValue = 0;
  tControlPositionValue m_recallModAmount = 0;
  ModulationSource m_recallModSource = ModulationSource::NONE;

  friend class RecallParameterSerializer;
};