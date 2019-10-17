#include "OverlayParameterLabel.h"

OverlayParameterLabel::OverlayParameterLabel(const Rect &r, const Parameter *parameter)
    : Label(r)
    , m_parameter{ parameter }
{
  if(parameter)
  {
    m_parameterChangedConnection
        = parameter->onParameterChanged(sigc::hide(sigc::mem_fun(this, &OverlayParameterLabel::updateText)));
  }

  setFontColor(FrameBuffer::C179);
}

void OverlayParameterLabel::updateText()
{
  setText({ m_parameter->getDisplayString(), 0 });
}

const Parameter *OverlayParameterLabel::getParameter() const
{
  return m_parameter;
}