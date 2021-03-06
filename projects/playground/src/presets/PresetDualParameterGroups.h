#pragma once

#include <http/UpdateDocumentContributor.h>
#include "AttributesOwner.h"
#include "GroupId.h"

class Preset;
class ParameterDualGroupSet;
class PresetParameterGroup;
class EditBuffer;

class PresetDualParameterGroups : public AttributesOwner
{
 public:
  PresetDualParameterGroups(UpdateDocumentContributor* parent);
  PresetDualParameterGroups(UpdateDocumentContributor* parent, const Preset& other);
  PresetDualParameterGroups(UpdateDocumentContributor* parent, const EditBuffer& eb);
  ~PresetDualParameterGroups() override;

  void writeDocument(Writer& writer, tUpdateID knownRevision) const override;
  void init(const Preset* preset);

 protected:
  using GroupPtr = std::unique_ptr<PresetParameterGroup>;
  using GroupsMap = std::map<GroupId, GroupPtr>;
  SoundType m_type;

  std::array<GroupsMap, static_cast<size_t>(VoiceGroup::NumGroups)> m_parameterGroups;

  friend class PresetParameterVoiceGroupSerializer;
  friend class PresetParameterGroupsSerializer;
};
