#include <proxies/hwui/panel-unit/boled/InfoContent.h>
#include <nltools/Assert.h>

InfoContent::InfoContent()
    : super(Rect(0, 0, 0, 64))
{
}

InfoContent::~InfoContent()
{
}

void InfoContent::setPosition(const Rect& rect)
{
  super::setPosition(rect);
  fixLayout();
}

const Rect& InfoContent::getPosition() const
{
  return super::getPosition();
}

void InfoContent::setDirty()
{
  super::setDirty();
  notifyDirty(true);
}

InfoContent::InfoField* InfoContent::addInfoField(std::string lineIdentifier, Glib::ustring labelText, Control* field)
{
  infoOrder.emplace_back(lineIdentifier);
  auto label = addControl(new SingleLineContent(labelText));
  addControl(field);
  infoFields[lineIdentifier] = std::make_unique<InfoField>(label, field);
  return infoFields[lineIdentifier].get();
}

InfoContent::InfoField* InfoContent::addInfoField(std::string lineIdentifier, Glib::ustring labelText)
{
  return addInfoField(lineIdentifier, labelText, new SingleLineContent());
}

InfoContent::SingleLineContent::SingleLineContent()
    : LeftAlignedLabel("---", Rect(0, 0, 0, 0))
{
}

InfoContent::SingleLineContent::SingleLineContent(Glib::ustring name)
    : LeftAlignedLabel(name, Rect(0, 0, 0, 0))
{
}

InfoContent::InfoField::InfoField(SingleLineContent* l, Control* c)
    : m_label(l)
    , m_content(c)
{
}

void InfoContent::InfoField::setInfo(Glib::ustring text, FrameBuffer::Colors c)
{
  if(auto label = dynamic_cast<SingleLineContent*>(m_content))
  {
    label->setFontColor(c);
    label->setText(text);
  }
  else if(auto multiLineLabel = dynamic_cast<MultiLineContent*>(m_content))
  {
    multiLineLabel->setText(text, c);
  }
  else
  {
    nltools_assertAlways(false);
  }
}

void InfoContent::InfoField::setInfo(Glib::ustring text)
{
  setInfo(text, FrameBuffer::Colors::C128);
}

int InfoContent::InfoField::format(int y)
{
  setPosition(y);
  auto height = std::max(m_content->getHeight(), m_label->getHeight());
  return y + height;
}

void InfoContent::InfoField::setPosition(int y)
{
  auto height = std::max(m_content->getHeight(), 16);
  m_label->setPosition(Rect(0, y, 64, 16));
  if(auto label = dynamic_cast<SingleLineContent*>(m_content))
  {
    label->setPosition(Rect(64, y, 256 - 64, height));
  }
  else if(auto multiLineLabel = dynamic_cast<MultiLineContent*>(m_content))
  {
    multiLineLabel->setPosition(Rect(64, y, 256 - 64, height));
  }
  else
  {
    nltools_assertAlways(false);
  }
}

InfoContent::MultiLineContent::MultiLineContent()
    : MultiLineLabel("---")
{
}

void InfoContent::MultiLineContent::setPosition(Rect rect)
{
  rect.moveBy(0, 2);
  MultiLineLabel::setPosition(rect);
}

void InfoContent::fixLayout()
{
  int y = 0;

  for(const auto& infoKey : infoOrder)
  {
    y = infoFields[infoKey]->format(y);
  }

  Rect r = getPosition();
  r.setHeight(y);
  ControlWithChildren::setPosition(r);
}

void InfoContent::updateContent()
{
  fillContents();
  fixLayout();
}
