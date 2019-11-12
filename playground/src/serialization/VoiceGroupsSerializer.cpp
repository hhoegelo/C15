#include <presets/EditBuffer.h>
#include "VoiceGroupsSerializer.h"
#include "ParameterGroupSerializer.h"

namespace Detail
{
  class VoiceGroupSerializer : public Serializer
  {
   public:
    VoiceGroupSerializer(EditBuffer *eb, VoiceGroup vg)
        : Serializer(tagName())
        , m_editBuffer{ eb }
        , m_voiceGroup{ vg }
    {
    }
    static std::string tagName()
    {
      return "param-groups";
    }

   protected:
    void writeTagContent(Writer &writer) const override
    {
      for(auto paramGroup : m_editBuffer->getParameterGroups(m_voiceGroup))
      {
        ParameterGroupSerializer group(paramGroup);
        group.write(writer, Attribute("id", paramGroup->getID()));
      }
    }

    void readTagContent(Reader &reader) const override
    {
      reader.onTag(ParameterGroupSerializer::getTagName(), [this](const auto attr) mutable {
        auto group = m_editBuffer->getParameterGroupByID(attr.get("id"), m_voiceGroup);
        if(group)
          return new ParameterGroupSerializer(group);
        return static_cast<ParameterGroupSerializer *>(nullptr);
      });
    }

   private:
    EditBuffer *m_editBuffer;
    VoiceGroup m_voiceGroup;
  };

  class GlobalParameterGroupsSerializer : public Serializer
  {
   public:
    GlobalParameterGroupsSerializer(EditBuffer *e)
        : Serializer(tagName())
        , m_editBuffer(e)
    {
    }

   protected:
    void writeTagContent(Writer &writer) const override
    {
      for(auto &g : m_editBuffer->getGlobalParameterGroups())
      {
        ParameterGroupSerializer ser(g);
        ser.write(writer);
      }
    }

    void readTagContent(Reader &reader) const override
    {
      reader.onTag(ParameterGroupSerializer::getTagName(), [this](const auto attr) mutable {
        auto group = m_editBuffer->getGlobalParameterGroupByID(attr.get("id"));
        if(group)
          return new ParameterGroupSerializer(group);
        return static_cast<ParameterGroupSerializer *>(nullptr);
      });
    }

   public:
    static std::string tagName()
    {
      return "Global-Groups";
    }

   private:
    EditBuffer *m_editBuffer;
  };
}

VoiceGroupsSerializer::VoiceGroupsSerializer(EditBuffer *editBuffer)
    : Serializer(getTagName())
    , m_editBuffer{ editBuffer }
{
}

Glib::ustring VoiceGroupsSerializer::getTagName()
{
  return "voice-groups";
}

void VoiceGroupsSerializer::writeTagContent(Writer &writer) const
{
  for(auto vg : { VoiceGroup::I, VoiceGroup::II })
  {
    writer.writeTag(toString(vg), [vg, &writer, this] {
      Detail::VoiceGroupSerializer s(m_editBuffer, vg);
      s.write(writer);
    });
  }

  Detail::GlobalParameterGroupsSerializer ser(m_editBuffer);
  ser.write(writer);
}

void VoiceGroupsSerializer::readTagContent(Reader &reader) const
{
  for(auto vg : { VoiceGroup::I, VoiceGroup::II })
  {
    reader.onTag(toString(vg),
                 [this, vg](const auto &attr) mutable { return new Detail::VoiceGroupSerializer(m_editBuffer, vg); });
  }

  reader.onTag(Detail::GlobalParameterGroupsSerializer::tagName(),
               [this](const auto &atttr) mutable { return new Detail::GlobalParameterGroupsSerializer(m_editBuffer); });
}
