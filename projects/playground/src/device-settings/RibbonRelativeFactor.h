#pragma once

#include "Setting.h"
#include <parameters/value/QuantizedValue.h>

class RibbonRelativeFactor : public Setting
{
 private:
  typedef Setting super;

 public:
  RibbonRelativeFactor(Settings &parent);
  virtual ~RibbonRelativeFactor();

  void load(const Glib::ustring &text) override;
  Glib::ustring save() const override;

  void set(tControlPositionValue amount);
  tControlPositionValue get() const;
  void setDefault();
  void incDec(int incs, ButtonModifiers modifiers);
  Glib::ustring getDisplayString() const;

  void writeDocument(Writer &writer, tUpdateID knownRevision) const override;
  void sendToLPC() const override;

 private:
  QuantizedValue m_factor;
};
