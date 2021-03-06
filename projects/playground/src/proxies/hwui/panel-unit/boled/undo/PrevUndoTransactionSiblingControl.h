#pragma once

#include "UndoEntryLabel.h"

class PrevUndoTransactionSiblingControl : public UndoEntryLabel
{
 private:
  typedef UndoEntryLabel super;

 public:
  PrevUndoTransactionSiblingControl(const Rect &r);
  virtual void assignTransaction(UNDO::Transaction *transaction, bool selected, bool current) override;

  std::shared_ptr<Font> getFont() const override;
  void setFontColor(FrameBuffer &fb) const override;

 private:
  bool m_bold = false;

  void setBold(bool bold);
};
