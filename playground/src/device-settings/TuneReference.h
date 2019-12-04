#pragma once

#include <parameters/value/QuantizedValue.h>
#include "Setting.h"
class TuneReference : public Setting
{
 public:
  TuneReference(Settings& parent);

 public:
  void load(const Glib::ustring& text) override;
  Glib::ustring save() const override;
  Glib::ustring getDisplayString() const override;
  void incDec(int inc, ButtonModifiers mod);

  Glib::ustring getName() const
  {
    return "Tune Reference";
  }

 private:
  void writeDocument(Writer& writer, tUpdateID knownRevision) const override;

  QuantizedValue m_value;
};
