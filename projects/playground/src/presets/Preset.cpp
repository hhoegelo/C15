#include <utility>
#include <presets/Preset.h>
#include "Bank.h"
#include <presets/PresetManager.h>
#include "PresetParameter.h"
#include "PresetParameterGroup.h"
#include <serialization/PresetSerializer.h>
#include <Application.h>
#include <device-settings/DebugLevel.h>
#include <device-settings/DeviceName.h>
#include <device-settings/Settings.h>
#include <device-info/DeviceInformation.h>
#include <tools/TimeTools.h>
#include <presets/EditBuffer.h>
#include <nltools/Types.h>
#include <groups/ParameterGroup.h>
#include <giomm/file.h>

Preset::Preset(UpdateDocumentContributor *parent)
    : super(parent)
    , m_name("New Preset")
    , m_voiceGroupLabels { { "", "" } }
{
}

Preset::Preset(UpdateDocumentContributor *parent, const Preset &other, bool ignoreUuids)
    : super(parent, other)
    , m_uuid(ignoreUuids ? Uuid() : other.m_uuid)
    , m_name(other.m_name)
    , m_voiceGroupLabels { other.m_voiceGroupLabels }
{
}

Preset::Preset(UpdateDocumentContributor *parent, const EditBuffer &editBuffer, bool copyUUID)
    : super(parent, editBuffer)
{
  m_name = editBuffer.getName();
  m_voiceGroupLabels[0] = editBuffer.getVoiceGroupName(VoiceGroup::I);
  m_voiceGroupLabels[1] = editBuffer.getVoiceGroupName(VoiceGroup::II);

  if(copyUUID)
    m_uuid = editBuffer.getUUIDOfLastLoadedPreset();
  else
    m_uuid.generate();
}

Preset::~Preset()
{
  if(auto pm = dynamic_cast<PresetManager *>(getParent()))
    if(auto eb = pm->getEditBuffer())
      eb->resetOriginIf(this);
}

void Preset::load(UNDO::Transaction *transaction, const Glib::RefPtr<Gio::File> &presetPath)
{
  auto strUUID = getUuid();
  Serializer::read<PresetSerializer>(transaction, presetPath, strUUID.raw(), this);
  m_lastSavedUpdateID = getUpdateIDOfLastChange();
}

bool Preset::save(const Glib::RefPtr<Gio::File> &bankPath)
{
  if(m_lastSavedUpdateID != getUpdateIDOfLastChange())
  {
    PresetSerializer serializer(this);
    auto strUUID = getUuid().raw();
    serializer.write(bankPath, strUUID);
    m_lastSavedUpdateID = getUpdateIDOfLastChange();
    return true;
  }
  return false;
}

UpdateDocumentContributor::tUpdateID Preset::onChange(uint64_t flags)
{
  auto ret = AttributesOwner::onChange(flags);
  m_onChanged.send();
  return ret;
}

void Preset::setAttribute(UNDO::Transaction *transaction, const std::string &key, const Glib::ustring &value)
{
  super::setAttribute(transaction, key, value);
  setAutoGeneratedAttributes(transaction);

  auto eb = getEditBuffer();

  if(eb && eb->getUUIDOfLastLoadedPreset() == getUuid() && !eb->isModified())
    eb->setAttribute(transaction, key, value);
}

EditBuffer *Preset::getEditBuffer() const
{
  if(Application::exists())
    return Application::get().getPresetManager()->getEditBuffer();

  return nullptr;
}

void Preset::copyFrom(UNDO::Transaction *transaction, const AttributesOwner *other)
{
  super::copyFrom(transaction, other);
  setAutoGeneratedAttributes(transaction);
}

void Preset::clear(UNDO::Transaction *transaction)
{
  super::clear(transaction);
  setAutoGeneratedAttributes(transaction);
}

void Preset::invalidate()
{
  onChange(ChangeFlags::Generic);
}

void Preset::updateBanksLastModifiedTimestamp(UNDO::Transaction *transaction)
{
  if(auto bank = dynamic_cast<Bank *>(getParent()))
    bank->updateLastModifiedTimestamp(transaction);
}

SoundType Preset::getType() const
{
  return m_type;
}

Glib::ustring Preset::getVoiceGroupName(VoiceGroup vg) const
{
  return m_voiceGroupLabels[static_cast<size_t>(vg)];
}

void Preset::undoableSetVoiceGroupName(UNDO::Transaction *transaction, VoiceGroup vg, const Glib::ustring &name)
{
  nltools_assertAlways(vg == VoiceGroup::I || vg == VoiceGroup::II);
  transaction->addUndoSwap(this, m_voiceGroupLabels[static_cast<size_t>(vg)], name);
}

const Uuid &Preset::getUuid() const
{
  return m_uuid;
}

Glib::ustring Preset::getDisplayNameWithSuffixes(bool addSpace) const
{
  auto mono = isMonoActive();
  auto unison = isUnisonActive();
  return getName() + (addSpace ? "\u202F" : "") + (mono ? "\uE040" : "") + (unison ? "\uE041" : "");
}

Glib::ustring Preset::getName() const
{
  return m_name;
}

void Preset::setUuid(UNDO::Transaction *transaction, const Uuid &uuid)
{
  transaction->addUndoSwap(this, m_uuid, uuid);
  setAutoGeneratedAttributes(transaction);
}

void Preset::setName(UNDO::Transaction *transaction, const Glib::ustring &name)
{
  transaction->addUndoSwap(this, m_name, name);
  setAutoGeneratedAttributes(transaction);
}

void Preset::setType(UNDO::Transaction *transaction, SoundType type)
{
  transaction->addUndoSwap(this, m_type, type);
  setAutoGeneratedAttributes(transaction);
}

void Preset::guessName(UNDO::Transaction *transaction)
{
  auto currentName = getName();

  if(currentName.empty())
    currentName = "New preset";

  setName(transaction, Application::get().getPresetManager()->createPresetNameBasedOn(currentName));
}

PresetParameter *Preset::findParameterByID(ParameterId id, bool throwIfMissing) const
{
  for(auto &g : m_parameterGroups[static_cast<size_t>(id.getVoiceGroup())])
    if(auto p = g.second->findParameterByID(id))
      return p;

  if(throwIfMissing)
    throw std::runtime_error("no such parameter" + id.toString() + " in " + toString(id.getVoiceGroup()));

  return nullptr;
}

PresetParameterGroup *Preset::findParameterGroup(const GroupId &id) const
{
  const auto &groups = m_parameterGroups.at(static_cast<size_t>(id.getVoiceGroup()));
  auto it = groups.find(id);
  if(it != groups.end())
    return it->second.get();

  return nullptr;
}

void Preset::forEachParameter(const std::function<void(PresetParameter *)> &cb)
{
  for(auto vg : { VoiceGroup::I, VoiceGroup::II, VoiceGroup::Global })
    for(auto &g : m_parameterGroups[static_cast<size_t>(vg)])
      for(auto &p : g.second->getParameters())
        cb(p.second.get());
}

void Preset::forEachParameter(const std::function<void(const PresetParameter *)> &cb) const
{
  for(auto vg : { VoiceGroup::I, VoiceGroup::II, VoiceGroup::Global })
    for(auto &g : m_parameterGroups[static_cast<size_t>(vg)])
      for(auto &p : g.second->getParameters())
        cb(p.second.get());
}

void Preset::copyFrom(UNDO::Transaction *transaction, const Preset *other, bool ignoreUuid)
{
  super::copyFrom(transaction, other);

  if(!ignoreUuid)
    setUuid(transaction, other->getUuid());

  setName(transaction, other->getName());
  undoableSetVoiceGroupName(transaction, VoiceGroup::I, other->getVoiceGroupName(VoiceGroup::I));
  undoableSetVoiceGroupName(transaction, VoiceGroup::II, other->getVoiceGroupName(VoiceGroup::II));

  setType(transaction, other->getType());

  for(auto vg : { VoiceGroup::I, VoiceGroup::II, VoiceGroup::Global })
    for(auto &g : m_parameterGroups[static_cast<size_t>(vg)])
      if(auto o = other->findParameterGroup(g.first))
        g.second->copyFrom(transaction, o);

  setAutoGeneratedAttributes(transaction);
}

void Preset::copyFrom(UNDO::Transaction *transaction, EditBuffer *edit)
{
  super::copyFrom(transaction, edit);
  setName(transaction, edit->getName());
  undoableSetVoiceGroupName(transaction, VoiceGroup::I, edit->getVoiceGroupName(VoiceGroup::I));
  undoableSetVoiceGroupName(transaction, VoiceGroup::II, edit->getVoiceGroupName(VoiceGroup::II));

  for(auto vg : { VoiceGroup::I, VoiceGroup::II, VoiceGroup::Global })
    for(auto g : edit->getParameterGroups(vg))
      m_parameterGroups[static_cast<size_t>(vg)][g->getID()] = std::make_unique<PresetParameterGroup>(*g);

  setType(transaction, edit->getType());
  setAutoGeneratedAttributes(transaction);
  getEditBuffer()->undoableSetLoadedPresetInfo(transaction, this);
}

void Preset::copyVoiceGroup1IntoVoiceGroup2(UNDO::Transaction *transaction, std::optional<std::set<GroupId>> whiteList)
{
  auto vgI = static_cast<size_t>(VoiceGroup::I);
  auto vgII = static_cast<size_t>(VoiceGroup::II);

  for(const auto &g : m_parameterGroups[vgI])
  {
    if(!whiteList || whiteList.value().count(g.first))
    {
      auto ptr = g.second.get();
      GroupId id { g.first.getName(), VoiceGroup::II };
      m_parameterGroups[vgII][id] = std::make_unique<PresetParameterGroup>(*ptr);
      m_parameterGroups[vgII][id]->assignVoiceGroup(transaction, VoiceGroup::II);
    }
  }
}

void Preset::setAutoGeneratedAttributes(UNDO::Transaction *transaction)
{
  if(m_autoGeneratedAttributesLockCount != 0)
    return;

  if(Application::exists())
  {
    super::setAttribute(transaction, "DeviceName", Application::get().getSettings()->getSetting<DeviceName>()->get());
    super::setAttribute(transaction, "StoreTime", TimeTools::getAdjustedIso());
    super::setAttribute(transaction, "SoftwareVersion",
                        Application::get().getDeviceInformation()->getSoftwareVersion());

    updateBanksLastModifiedTimestamp(transaction);
  }
}

Glib::ustring Preset::buildUndoTransactionTitle(const Glib::ustring &prefix) const
{
  if(auto bank = static_cast<const Bank *>(getParent()))
  {
    if(auto pm = static_cast<const PresetManager *>(bank->getParent()))
    {
      auto bankNumber = pm->getBankPosition(bank->getUuid()) + 1;
      auto presetNumber = bank->getPresetPosition(getUuid()) + 1;

      char txt[256];
      sprintf(txt, "%lu-%03lu", bankNumber, presetNumber);
      return UNDO::StringTools::buildString(prefix, " ", toString(getType()), " Preset ", txt, ": '", getName(), "'");
    }
  }

  return UNDO::StringTools::buildString(prefix, " '", getName(), "'");
}

bool Preset::matchesQuery(const SearchQuery &query) const
{
  return query.iterate([&](const auto &part, const auto &fields) {
    static_assert(static_cast<int>(SearchQuery::Fields::Name) == 0, "");
    static_assert(static_cast<int>(SearchQuery::Fields::Comment) == 1, "");
    static_assert(static_cast<int>(SearchQuery::Fields::DeviceName) == 2, "");

    std::array<Glib::ustring, 3> entries = { getName().lowercase(), getAttribute("Comment", "").lowercase(),
                                             getAttribute("DeviceName", "").lowercase() };

    for(const auto f : fields)
    {
      if(entries[static_cast<size_t>(f)].find(part) != Glib::ustring::npos)
      {
        return true;
      }
    }
    return false;
  });
}

void Preset::lockAutoGeneratedAttributes()
{
  m_autoGeneratedAttributesLockCount++;
}

void Preset::unlockAutoGeneratedAttributes()
{
  m_autoGeneratedAttributesLockCount--;
}

sigc::connection Preset::onChanged(sigc::slot<void> cb)
{
  return m_onChanged.connectAndInit(cb);
}

void Preset::writeDocument(Writer &writer, UpdateDocumentContributor::tUpdateID knownRevision) const
{
  bool changed = knownRevision < getUpdateIDOfLastChange();

  writer.writeTag("preset",
                  {
                      Attribute("uuid", m_uuid.raw()),
                      Attribute("name", m_name),
                      Attribute("name-suffixed", getDisplayNameWithSuffixes(true)),
                      Attribute("changed", changed),
                      Attribute("type", toString(m_type)),
                      Attribute("part-I-name", getVoiceGroupName(VoiceGroup::I)),
                      Attribute("part-II-name", getVoiceGroupName(VoiceGroup::II)),
                  },
                  [&]() {
                    if(changed)
                    {
                      AttributesOwner::writeDocument(writer, knownRevision);
                    }
                  });
}

void Preset::writeDiff(Writer &writer, const Preset *other, VoiceGroup vgOfThis, VoiceGroup vgOfOther) const
{
  auto pm = Application::get().getPresetManager();

  auto posString = [&](const Preset *p) {
    std::string ret;
    if(auto b = dynamic_cast<Bank *>(p->getParent()))
    {
      char txt[256];
      sprintf(txt, "%lu-%03lu", pm->getBankPosition(b->getUuid()) + 1, b->getPresetPosition(p->getUuid()) + 1);
      ret = txt;
    }
    else
      ret = "Edit Buffer";
    return ret;
  };

  auto eb = pm->getEditBuffer();
  const auto ebUUID = eb->getUUIDOfLastLoadedPreset();

  auto getButtonStatesPresets = [&](const Preset *p, const Preset *other) {
    std::pair<bool, bool> active;
    auto ebChanged = eb->isModified();
    active.first = p->getUuid() != ebUUID || ebChanged;
    active.second = other->getUuid() != ebUUID || ebChanged;
    return active;
  };

  auto getButtonStates = [&](const Preset *p, const Preset *other) {
    std::pair<bool, bool> active;
    auto isLoaded = (p->getUuid() == ebUUID);
    active.first = !isLoaded || eb->isModified();
    active.second = eb->isModified() & !active.first;
    return active;
  };

  writer.writeTag("diff", [&] {
    std::pair<bool, bool> buttonStates;
    if(posString(this) == "Edit Buffer" || posString(other) == "Edit Buffer")
      buttonStates = getButtonStates(this, other);
    else
      buttonStates = getButtonStatesPresets(this, other);

    writer.writeTextElement("position", "", Attribute("a", posString(this)), Attribute("b", posString(other)));
    writer.writeTextElement("name", "", Attribute("a", getName()), Attribute("b", other->getName()));
    writer.writeTextElement("enabled", "", Attribute("a", buttonStates.first), Attribute("b", buttonStates.second));

    super::writeDiff(writer, other);

    writeGroups(writer, other, vgOfThis, vgOfOther);
  });
}

void Preset::writeGroups(Writer &writer, const Preset *other, VoiceGroup vgOfThis, VoiceGroup vgOfOther) const
{
  for(auto &g : m_parameterGroups[static_cast<size_t>(vgOfThis)])
    g.second->writeDiff(writer, g.first, other->findParameterGroup({ g.first.getName(), vgOfOther }));
}

PresetParameterGroup *Preset::findOrCreateParameterGroup(const GroupId &id)
{
  if(auto ret = findParameterGroup(id))
  {
    return ret;
  }
  else
  {
    auto &vgMap = m_parameterGroups[static_cast<size_t>(id.getVoiceGroup())];
    vgMap[id] = std::make_unique<PresetParameterGroup>(id.getVoiceGroup());
    return findParameterGroup(id);
  }
}

bool Preset::isMonoActive() const
{
  auto monoEnabledI = findParameterByID({ 364, VoiceGroup::I }, false);
  auto monoEnabledII = findParameterByID({ 364, VoiceGroup::II }, false);

  if(monoEnabledI && monoEnabledII)
  {
    if(getType() == SoundType::Split)
    {
      return monoEnabledI->getValue() > 0 || monoEnabledII->getValue() > 0;
    }
    else
    {
      return monoEnabledI->getValue() > 0;
    }
  }

  return false;
}

bool Preset::isUnisonActive() const
{
  auto unisonVoicesI = findParameterByID({ 249, VoiceGroup::I }, false);
  auto unisonVoicesII = findParameterByID({ 249, VoiceGroup::II }, false);

  if(unisonVoicesI)
  {
    if(getType() == SoundType::Split && unisonVoicesII)
    {
      return unisonVoicesI->getValue() > 0 || unisonVoicesII->getValue() > 0;
    }
    else
    {
      return unisonVoicesI->getValue() > 0;
    }
  }

  return false;
}

bool Preset::isDual() const
{
  return getType() == SoundType::Split || getType() == SoundType::Layer;
}
