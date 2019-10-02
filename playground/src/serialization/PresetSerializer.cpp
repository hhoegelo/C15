#include "AttributesOwnerSerializer.h"
#include "PresetSerializer.h"
#include "PresetParameterGroupsSerializer.h"
#include <presets/Preset.h>

PresetSerializer::PresetSerializer(Preset *preset, bool ignoreUUIDs)
    : Serializer(getTagName())
    , m_preset(preset)
    , m_ignoreUUIDs(ignoreUUIDs)
    , m_presetLock(preset)
{
}

PresetSerializer::~PresetSerializer()
{
}

Glib::ustring PresetSerializer::getTagName()
{
  return "preset";
}

void PresetSerializer::writeTagContent(Writer &writer) const
{
  writer.writeTextElement("name", m_preset->getName());
  writer.writeTextElement("uuid", m_preset->getUuid().raw());
  writer.writeTextElement("type", toString(m_preset->getType()));

  AttributesOwnerSerializer attributesWriter(m_preset);
  attributesWriter.write(writer);

  PresetParameterGroupsSerializer groups(m_preset);
  groups.write(writer);
}

void PresetSerializer::readTagContent(Reader &reader) const
{
  reader.onTextElement("name", [&](auto &text, auto) { m_preset->setName(reader.getTransaction(), text); });

  reader.onTextElement("type",
                       [&](auto &text, auto) { m_preset->setType(reader.getTransaction(), toSoundType(text)); });

  if(!m_ignoreUUIDs)
  {
    reader.onTextElement("uuid", [&](const Glib::ustring &text, const Attributes &attr) {
      m_preset->setUuid(reader.getTransaction(), text);
    });
  }

  reader.onTag(AttributesOwnerSerializer::getTagName(),
               [&](const Attributes &attr) mutable { return new AttributesOwnerSerializer(m_preset); });

  reader.onTag(PresetParameterGroupsSerializer::getTagName(),
               [&](const Attributes &attr) mutable { return new PresetParameterGroupsSerializer(m_preset); });
}
