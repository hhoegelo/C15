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

Preset::Preset(UpdateDocumentContributor *parent)
    : super(parent)
{
}

Preset::Preset(UpdateDocumentContributor *parent, const Preset &other, bool ignoreUuids)
    : super(parent, other)
    , m_uuid(ignoreUuids ? Uuid() : other.m_uuid)
    , m_name(other.m_name)
{
}

Preset::Preset(UpdateDocumentContributor *parent, const EditBuffer &editBuffer)
    : super(parent, editBuffer)
{
  m_name = editBuffer.getName();
}

Preset::~Preset()
{
}

void Preset::load(UNDO::Transaction *transaction, RefPtr<Gio::File> presetPath)
{
  auto strUUID = getUuid();
  Serializer::read<PresetSerializer>(transaction, presetPath, strUUID.raw(), this);
  m_lastSavedUpdateID = getUpdateIDOfLastChange();
}

bool Preset::save(RefPtr<Gio::File> bankPath)
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

void Preset::setAttribute(UNDO::Transaction *transaction, const string &key, const ustring &value)
{
  super::setAttribute(transaction, key, value);
  setAutoGeneratedAttributes(transaction);

  if(auto eb = getEditBuffer())
    if(eb->getUUIDOfLastLoadedPreset() == getUuid())
      eb->setAttribute(transaction, key, value);
}

EditBuffer *Preset::getEditBuffer()
{
  if(auto bank = dynamic_cast<Bank *>(getParent()))
    if(auto pm = dynamic_cast<PresetManager *>(bank->getParent()))
      return pm->getEditBuffer();

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

const Uuid &Preset::getUuid() const
{
  return m_uuid;
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

void Preset::guessName(UNDO::Transaction *transaction)
{
  auto currentName = getName();

  if(currentName.empty())
    currentName = "New preset";

  setName(transaction, Application::get().getPresetManager()->createPresetNameBasedOn(currentName));
}

PresetParameter *Preset::findParameterByID(int id) const
{
  for(auto &g : m_parameterGroups)
    if(auto p = g.second->findParameterByID(id))
      return p;

  throw std::runtime_error("no such parameter");
}

PresetParameterGroup *Preset::findParameterGroup(const string &id) const
{
  auto it = m_parameterGroups.find(id);

  if(it != m_parameterGroups.end())
    return it->second.get();

  return nullptr;
}

void Preset::copyFrom(UNDO::Transaction *transaction, const Preset *other, bool ignoreUuid)
{
  super::copyFrom(transaction, other);

  if(!ignoreUuid)
    setUuid(transaction, other->getUuid());

  setName(transaction, other->getName());

  for(auto &g : m_parameterGroups)
    if(auto o = other->findParameterGroup(g.first))
      g.second->copyFrom(transaction, o);

  setAutoGeneratedAttributes(transaction);
}

void Preset::copyFrom(UNDO::Transaction *transaction, EditBuffer *edit)
{
  super::copyFrom(transaction, edit);
  setName(transaction, edit->getName());

  for(auto g : edit->getParameterGroups())
    m_parameterGroups[g->getID()] = std::make_unique<PresetParameterGroup>(*g);

  setAutoGeneratedAttributes(transaction);
  getEditBuffer()->undoableSetLoadedPresetInfo(transaction, this);
}

void Preset::setAutoGeneratedAttributes(UNDO::Transaction *transaction)
{
  super::setAttribute(transaction, "DeviceName", Application::get().getSettings()->getSetting<DeviceName>()->get());
  super::setAttribute(transaction, "StoreTime", TimeTools::getAdjustedIso());
  super::setAttribute(transaction, "SoftwareVersion", Application::get().getDeviceInformation()->getSoftwareVersion());

  updateBanksLastModifiedTimestamp(transaction);
}

ustring Preset::buildUndoTransactionTitle(const ustring &prefix) const
{
  if(auto bank = static_cast<const Bank *>(getParent()))
  {
    if(auto pm = static_cast<const PresetManager *>(bank->getParent()))
    {
      auto bankNumber = pm->getBankPosition(bank->getUuid()) + 1;
      auto presetNumber = bank->getPresetPosition(getUuid()) + 1;

      char txt[256];
      sprintf(txt, "%zu-%03zu", bankNumber, presetNumber);
      return UNDO::StringTools::buildString(prefix, " ", txt, ": '", getName(), "'");
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

    std::array<ustring, 3> entries = { getName().lowercase(), getAttribute("Comment", "").lowercase(),
                                       getAttribute("DeviceName", "").lowercase() };

    for(auto f : fields)
    {
      if(entries[static_cast<size_t>(f)].find(part) != ustring::npos)
      {
        return true;
      }
    }
    return false;
  });
}

connection Preset::onChanged(sigc::slot<void> cb)
{
  return m_onChanged.connectAndInit(cb);
}

void Preset::writeDocument(Writer &writer, UpdateDocumentContributor::tUpdateID knownRevision) const
{
  bool changed = knownRevision < getUpdateIDOfLastChange();

  writer.writeTag("preset",
                  { Attribute("uuid", m_uuid.raw()), Attribute("name", m_name), Attribute("changed", changed) },
                  [&]() {
                    if(changed)
                    {
                      AttributesOwner::writeDocument(writer, knownRevision);
                    }
                  });
}


void Preset::writeDetailDocument(Writer& writer, UpdateDocumentContributor::tUpdateID knownRevision) const {
    PresetParameterGroups::writeDocument(writer, knownRevision);
};

void Preset::writeDiff(Writer &writer, const Preset *other) const
{
  char txt[256];
  auto pm = Application::get().getPresetManager();
  std::string aPositionString;
  std::string bPositionString;

  if(auto b = dynamic_cast<Bank *>(getParent()))
  {
    sprintf(txt, "%zu-%03zu", pm->getBankPosition(b->getUuid()) + 1, b->getPresetPosition(getUuid()) + 1);
    aPositionString = txt;
  }

  if(auto b = dynamic_cast<Bank *>(other->getParent()))
  {
    sprintf(txt, "%zu-%03zu", pm->getBankPosition(b->getUuid()) + 1, b->getPresetPosition(other->getUuid()) + 1);
    bPositionString = txt;
  }

  writer.writeTag("diff", [&] {
    writer.writeTextElement("position", "", Attribute("a", aPositionString), Attribute("b", bPositionString));
    writer.writeTextElement("name", "", Attribute("a", getName()), Attribute("b", other->getName()));

    super::writeDiff(writer, other);

    for(auto &group : m_parameterGroups)
      group.second->writeDiff(writer, group.first, other->findParameterGroup(group.first));
  });
}
