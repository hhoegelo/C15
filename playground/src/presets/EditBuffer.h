#pragma once

#include "ParameterGroupSet.h"
#include <libsoup/soup.h>
#include "Preset.h"
#include "LastLoadedPresetInfo.h"
#include <tools/Expiration.h>

class Application;
class Writer;
class Preset;
class PresetManager;

class EditBuffer : public Preset
{
  private:
    typedef Preset super;

  public:
    static shared_ptr<EditBuffer> createEditBuffer (UpdateDocumentContributor *parent);
    virtual ~EditBuffer ();

    void undoableClear (UNDO::Scope::tTransactionPtr transaction);

    void undoableSelectParameter (const Glib::ustring &id);
    void undoableSelectParameter (uint16_t id);
    void undoableSelectParameter (Parameter *p);
    Parameter *getSelected () const;

    void undoableLoad (UNDO::Scope::tTransactionPtr transaction, shared_ptr<Preset> preset);
    void undoableLoad (shared_ptr<Preset> preset);
    void undoableLoadSelectedPreset ();
    void undoableSetLoadedPresetInfo (UNDO::Scope::tTransactionPtr transaction, shared_ptr<Preset> preset);
    void undoableUpdateLoadedPresetInfo (UNDO::Scope::tTransactionPtr transaction);
    void undoableRandomize(UNDO::Scope::tTransactionPtr transaction, Initiator initiator);
    void undoableInitSound (UNDO::Scope::tTransactionPtr transaction);
    void undoableSetDefaultValues (UNDO::Scope::tTransactionPtr transaction, Preset *values);

    void writeDocument (Writer &writer, tUpdateID knownRevision) const override;
    Glib::ustring getUUIDOfLastLoadedPreset () const;
    PresetManager *getParent ();
    const PresetManager *getParent () const;

    void resetModifiedIndicator (UNDO::Scope::tTransactionPtr transaction);
    virtual void copyFrom (UNDO::Scope::tTransactionPtr transaction, Preset *other, bool ignoreUUIDs) override;

    virtual tUpdateID onChange () override;

    // CALLBACKS
    sigc::connection onSelectionChanged (slot<void, Parameter *, Parameter *> s);
    sigc::connection onModificationStateChanged (slot<void, bool> s);
    sigc::connection onChange (slot<void> s);

    void undoableImportReaktorPreset (const Glib::ustring &preset);
    void undoableImportReaktorPreset (UNDO::Scope::tTransactionPtr transaction, const Glib::ustring &preset);
    Glib::ustring exportReaktorPreset ();
    bool isModified () const;
    void sendToLPC ();
    bool isSelectedPresetLoadedAndUnModified();

  private:
    virtual UNDO::Scope &getUndoScope () override;

    void setParameter (size_t id, double cpValue);

    EditBuffer (UpdateDocumentContributor *parent);

    void undoableSelectParameter (UNDO::Scope::tTransactionPtr transaction, Parameter *p);
    void undoableSelectParameter (UNDO::Scope::tTransactionPtr transaction, const Glib::ustring &id);

    void setModulationSource (int src);
    void setModulationAmount (double amount);

    bool undoableImportReaktorParameter (UNDO::Scope::tTransactionPtr transaction, std::istringstream &input, Parameter *param);
    bool readReaktorPresetHeader (std::istringstream &input) const;

    void checkModified ();

    Signal<void, Parameter *, Parameter *> m_signalSelectedParameter;
    Signal<void, bool> m_signalModificationState;
    Signal<void> m_signalChange;

    Parameter *m_selectedParameter = nullptr;

    friend class EditBufferSerializer;
    friend class EditBufferActions;

    LastLoadedPresetInfo m_lastLoadedPresetInfo;

    size_t m_hashOnStore;
    bool m_isModified;
    Expiration m_checkModified;

    friend class PresetManager;
};
