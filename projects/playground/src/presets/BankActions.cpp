#include "BankActions.h"

#include <serialization/EditBufferSerializer.h>
#include <serialization/PresetBankSerializer.h>
#include <serialization/PresetSerializer.h>
#include <presets/PresetManager.h>
#include <presets/Bank.h>
#include <presets/Preset.h>
#include "EditBuffer.h"
#include "http/SoupOutStream.h"
#include <http/HTTPRequest.h>
#include "xml/XmlWriter.h"
#include "ClusterEnforcement.h"
#include <xml/MemoryInStream.h>
#include <xml/XmlReader.h>
#include <device-settings/DirectLoadSetting.h>
#include <device-settings/Settings.h>
#include <device-info/DateTimeInfo.h>
#include <Application.h>
#include <tools/PerformanceTimer.h>
#include <xml/VersionAttribute.h>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <tools/TimeTools.h>
#include <proxies/hwui/HWUI.h>
#include <nltools/Assert.h>

BankActions::BankActions(PresetManager &presetManager)
    : RPCActionManager("/presets/banks/")
    , m_presetManager(presetManager)
{
  addAction("drop-presets-above", [&](std::shared_ptr<NetworkRequest> request) {
    UNDO::Scope::tTransactionScopePtr scope = m_presetManager.getUndoScope().startTransaction("Drop Presets");
    auto transaction = scope->getTransaction();

    Glib::ustring csv = request->get("presets");
    Glib::ustring anchorUUID = request->get("anchor");

    std::vector<std::string> strs;
    boost::split(strs, csv, boost::is_any_of(","));

    dropPresets(transaction, anchorUUID, 0, csv);
  });

  addAction("drop-presets-below", [&](std::shared_ptr<NetworkRequest> request) {
    UNDO::Scope::tTransactionScopePtr scope = m_presetManager.getUndoScope().startTransaction("Drop Presets");
    auto transaction = scope->getTransaction();

    Glib::ustring csv = request->get("presets");
    Glib::ustring anchorUUID = request->get("anchor");

    dropPresets(transaction, anchorUUID, 1, csv);
  });

  addAction("drop-presets-to", [&](std::shared_ptr<NetworkRequest> request) {
    UNDO::Scope::tTransactionScopePtr scope = m_presetManager.getUndoScope().startTransaction("Drop Presets");
    auto transaction = scope->getTransaction();

    Glib::ustring csv = request->get("presets");
    Glib::ustring anchorUUID = request->get("anchor");

    dropPresets(transaction, anchorUUID, 1, csv);

    if(auto anchor = presetManager.findPreset(anchorUUID))
    {
      dynamic_cast<Bank *>(anchor->getParent())->deletePreset(transaction, anchorUUID);
    }
  });

  addAction("rename-preset", [&](std::shared_ptr<NetworkRequest> request) {
    Glib::ustring uuid = request->get("uuid");
    Glib::ustring newName = request->get("name");

    if(auto p = m_presetManager.findPreset(uuid))
    {
      if(p->getName() != newName)
      {
        UNDO::Scope::tTransactionScopePtr scope
            = m_presetManager.getUndoScope().startTransaction("Rename preset '%0' to '%1'", p->getName(), newName);
        p->setName(scope->getTransaction(), newName);
        p->setAutoGeneratedAttributes(scope->getTransaction());
        m_presetManager.getEditBuffer()->undoableUpdateLoadedPresetInfo(scope->getTransaction());
      }
    }
  });

  addAction("move-preset-above", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto presetToMoveUuid = request->get("presetToMove");
    auto presetAnchorUuid = request->get("anchor");
    auto srcBank = m_presetManager.findBankWithPreset(presetToMoveUuid);
    auto tgtBank = m_presetManager.findBankWithPreset(presetAnchorUuid);

    if(srcBank && tgtBank)
    {
      if(auto toMove = srcBank->findPreset(presetToMoveUuid))
      {
        auto scope = m_presetManager.getUndoScope().startTransaction("Move preset");
        auto transaction = scope->getTransaction();
        auto anchor = tgtBank->findPresetNear(presetAnchorUuid, 0);
        srcBank->movePresetBetweenBanks(transaction, toMove, tgtBank, anchor);
        tgtBank->selectPreset(transaction, presetToMoveUuid);
        m_presetManager.selectBank(transaction, tgtBank->getUuid());
      }
    }
  });

  addAction("move-preset-below", [&](std::shared_ptr<NetworkRequest> request) {
    auto presetToMoveUuid = request->get("presetToMove");
    auto presetAnchorUuid = request->get("anchor");
    auto srcBank = m_presetManager.findBankWithPreset(presetToMoveUuid);
    auto tgtBank = m_presetManager.findBankWithPreset(presetAnchorUuid);

    if(srcBank && tgtBank)
    {
      if(auto toMove = srcBank->findPreset(presetToMoveUuid))
      {
        auto scope = m_presetManager.getUndoScope().startTransaction("Move preset");
        auto transaction = scope->getTransaction();
        auto anchor = tgtBank->findPresetNear(presetAnchorUuid, 1);
        srcBank->movePresetBetweenBanks(transaction, toMove, tgtBank, anchor);
        tgtBank->selectPreset(transaction, presetToMoveUuid);
        m_presetManager.selectBank(transaction, tgtBank->getUuid());
      }
    }
  });

  addAction("move-preset-to", [&](std::shared_ptr<NetworkRequest> request) {
    auto presetToOverwrite = request->get("presetToOverwrite");
    auto overwriteWith = request->get("overwriteWith");
    auto tgtBank = m_presetManager.findBankWithPreset(presetToOverwrite);
    auto srcBank = m_presetManager.findBankWithPreset(overwriteWith);

    if(srcBank && tgtBank)
    {
      auto srcPreset = srcBank->findPreset(overwriteWith);
      auto tgtPreset = tgtBank->findPreset(presetToOverwrite);

      if(srcPreset != tgtPreset)
      {
        auto scope = m_presetManager.getUndoScope().startTransaction("Overwrite preset");
        auto transaction = scope->getTransaction();

        auto anchor = tgtBank->findPresetNear(presetToOverwrite, 0);
        tgtBank->movePresetBetweenBanks(transaction, srcPreset, tgtBank, anchor);
        srcBank->deletePreset(transaction, presetToOverwrite);
        tgtBank->selectPreset(transaction, srcPreset->getUuid());
        m_presetManager.selectBank(transaction, tgtBank->getUuid());
      }
    }
  });

  addAction("overwrite-preset", [&](std::shared_ptr<NetworkRequest> request) {
    auto presetToOverwrite = request->get("presetToOverwrite");
    auto overwriteWith = request->get("overwriteWith");
    auto scope = m_presetManager.getUndoScope().startTransaction("Overwrite preset");
    auto transaction = scope->getTransaction();

    if(presetToOverwrite.empty())
    {
      if(!m_presetManager.getSelectedBank())
      {
        nltools_assertAlways(m_presetManager.getNumBanks() == 0);
        auto b = m_presetManager.addBank(transaction);
        m_presetManager.selectBank(transaction, b->getUuid());
      }

      auto selectedBank = m_presetManager.getSelectedBank();
      if(!selectedBank->getSelectedPreset())
      {
        nltools_assertAlways(selectedBank->getNumPresets() == 0);
        auto p = selectedBank->appendPreset(transaction);
        selectedBank->selectPreset(transaction, p->getUuid());
      }

      if(presetToOverwrite.empty())
        presetToOverwrite = selectedBank->getSelectedPresetUuid().raw();
    }

    if(overwriteWith.empty())
    {
      if(auto tgtPreset = m_presetManager.findPreset(presetToOverwrite))
      {
        tgtPreset->copyFrom(transaction, m_presetManager.getEditBuffer());
        auto bank = dynamic_cast<Bank *>(tgtPreset->getParent());
        bank->selectPreset(transaction, tgtPreset->getUuid());
        m_presetManager.selectBank(transaction, bank->getUuid());
      }
    }
    else
    {
      auto srcPreset = m_presetManager.findPreset(overwriteWith);
      auto tgtPreset = m_presetManager.findPreset(presetToOverwrite);

      if(srcPreset && tgtPreset)
      {
        tgtPreset->copyFrom(transaction, srcPreset, true);
        auto bank = dynamic_cast<Bank *>(tgtPreset->getParent());
        bank->selectPreset(transaction, tgtPreset->getUuid());
        m_presetManager.selectBank(transaction, bank->getUuid());
      }
    }
  });

  addAction("copy-preset-above", [&](std::shared_ptr<NetworkRequest> request) {
    auto presetToMove = request->get("presetToCopy");
    auto presetAnchor = request->get("anchor");
    auto srcBank = m_presetManager.findBankWithPreset(presetToMove);
    auto tgtBank = m_presetManager.findBankWithPreset(presetAnchor);

    if(srcBank && tgtBank)
    {
      auto srcPreset = srcBank->findPreset(presetToMove);
      auto anchorPos = tgtBank->getPresetPosition(presetAnchor);

      nltools_assertAlways(srcPreset);

      auto scope = m_presetManager.getUndoScope().startTransaction("Copy preset '%0'", srcPreset->getName());
      auto transaction = scope->getTransaction();
      auto newPreset = std::make_unique<Preset>(tgtBank, *srcPreset, true);
      auto tgtPreset = tgtBank->insertPreset(transaction, anchorPos, std::move(newPreset));
      tgtBank->selectPreset(transaction, tgtPreset->getUuid());
      m_presetManager.selectBank(transaction, tgtBank->getUuid());
    }
  });

  addAction("copy-preset-below", [&](std::shared_ptr<NetworkRequest> request) {
    auto presetToMove = request->get("presetToCopy");
    auto presetAnchor = request->get("anchor");
    auto srcBank = m_presetManager.findBankWithPreset(presetToMove);
    auto tgtBank = m_presetManager.findBankWithPreset(presetAnchor);

    if(srcBank && tgtBank)
    {
      auto srcPreset = srcBank->findPreset(presetToMove);
      auto anchorPos = tgtBank->getPresetPosition(presetAnchor) + 1;

      nltools_assertAlways(srcPreset);

      auto scope = m_presetManager.getUndoScope().startTransaction("Copy preset '%0'", srcPreset->getName());
      auto transaction = scope->getTransaction();
      auto newPreset = std::make_unique<Preset>(tgtBank, *srcPreset, true);
      auto tgtPreset = tgtBank->insertPreset(transaction, anchorPos, std::move(newPreset));
      tgtBank->selectPreset(transaction, tgtPreset->getUuid());
      m_presetManager.selectBank(transaction, tgtBank->getUuid());
    }
  });

  addAction("insert-editbuffer-above", [&](std::shared_ptr<NetworkRequest> request) {
    auto presetAnchor = request->get("anchor");
    auto uuid = request->get("uuid");

    if(auto tgtBank = m_presetManager.findBankWithPreset(presetAnchor))
    {
      auto name = guessNameBasedOnEditBuffer();
      auto anchorPos = tgtBank->getPresetPosition(presetAnchor);
      auto &undoScope = m_presetManager.getUndoScope();
      auto transactionScope = undoScope.startTransaction("Save new Preset in Bank '%0'", tgtBank->getName(true));
      auto transaction = transactionScope->getTransaction();
      auto newPreset = std::make_unique<Preset>(tgtBank, *m_presetManager.getEditBuffer());
      auto tgtPreset = tgtBank->insertAndLoadPreset(transaction, anchorPos, std::move(newPreset));
      tgtPreset->setName(transaction, name);

      if(!uuid.empty())
        tgtPreset->setUuid(transaction, uuid);

      tgtBank->selectPreset(transaction, tgtPreset->getUuid());
      m_presetManager.selectBank(transaction, tgtBank->getUuid());
    }
  });

  addAction("insert-editbuffer-below", [&](std::shared_ptr<NetworkRequest> request) {
    auto presetAnchor = request->get("anchor");
    auto uuid = request->get("uuid");

    if(auto tgtBank = m_presetManager.findBankWithPreset(presetAnchor))
    {
      auto name = guessNameBasedOnEditBuffer();
      auto anchorPos = tgtBank->getPresetPosition(presetAnchor) + 1;
      auto &undoScope = m_presetManager.getUndoScope();
      auto transactionScope = undoScope.startTransaction("Save new Preset in Bank '%0'", tgtBank->getName(true));
      auto transaction = transactionScope->getTransaction();
      auto newPreset = std::make_unique<Preset>(tgtBank, *m_presetManager.getEditBuffer());
      auto tgtPreset = tgtBank->insertAndLoadPreset(transaction, anchorPos, std::move(newPreset));
      tgtPreset->setName(transaction, name);

      if(!uuid.empty())
        tgtPreset->setUuid(transaction, uuid);

      tgtBank->selectPreset(transaction, tgtPreset->getUuid());
      m_presetManager.selectBank(transaction, tgtBank->getUuid());
    }
  });

  addAction("overwrite-preset-with-editbuffer", [&](std::shared_ptr<NetworkRequest> request) {
    auto presetToOverwrite = request->get("presetToOverwrite");

    if(auto tgtBank = m_presetManager.findBankWithPreset(presetToOverwrite))
    {
      if(auto tgtPreset = tgtBank->findPreset(presetToOverwrite))
      {
        auto name = tgtPreset->getName();
        auto ebUUID = Application::get().getPresetManager()->getEditBuffer()->getUUIDOfLastLoadedPreset();

        if(ebUUID != presetToOverwrite)
          name = guessNameBasedOnEditBuffer();

        auto scope = m_presetManager.getUndoScope().startTransaction("Overwrite preset '%0'", tgtPreset->getName());
        auto transaction = scope->getTransaction();
        tgtPreset->copyFrom(transaction, m_presetManager.getEditBuffer());
        tgtPreset->setName(transaction, name);
        tgtBank->selectPreset(transaction, tgtPreset->getUuid());
        m_presetManager.selectBank(transaction, tgtBank->getUuid());
      }
    }
  });

  addAction("append-preset", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto b = m_presetManager.getSelectedBank();
    std::string fallBack = b ? b->getUuid().raw() : "";
    auto bankToAppendTo = request->get("bank-uuid", fallBack);
    auto uuid = request->get("uuid");

    if(auto bank = m_presetManager.findBank(bankToAppendTo))
    {
      auto &undoScope = m_presetManager.getUndoScope();
      auto scope = undoScope.startTransaction("Append Preset to Bank '%0'", bank->getName(true));
      auto transaction = scope->getTransaction();
      auto newName = guessNameBasedOnEditBuffer();
      auto newPreset = std::make_unique<Preset>(bank, *m_presetManager.getEditBuffer());
      auto tgtPreset = bank->appendAndLoadPreset(transaction, std::move(newPreset));

      tgtPreset->setName(transaction, newName);

      if(!uuid.empty())
        tgtPreset->setUuid(transaction, uuid);

      bank->selectPreset(transaction, tgtPreset->getUuid());
      m_presetManager.selectBank(transaction, bank->getUuid());
    }
  });

  addAction("append-preset-to-bank", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto bankUuid = request->get("bank-uuid");
    auto presetUuid = request->get("preset-uuid");

    if(auto bank = m_presetManager.findBank(bankUuid))
    {
      if(auto srcPreset = m_presetManager.findPreset(presetUuid))
      {
        auto &undoScope = m_presetManager.getUndoScope();
        auto scope = undoScope.startTransaction("Append Preset to Bank '%0'", bank->getName(true));
        auto transaction = scope->getTransaction();

        auto newPreset = std::make_unique<Preset>(bank, *m_presetManager.getEditBuffer());
        auto tgtPreset = bank->appendAndLoadPreset(transaction, std::move(newPreset));

        bank->selectPreset(transaction, tgtPreset->getUuid());
        m_presetManager.selectBank(transaction, bank->getUuid());
      }
    }
  });

  addAction("set-order-number", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto uuid = request->get("uuid");
    if(auto bank = m_presetManager.findBank(uuid))
    {
      auto &undoScope = m_presetManager.getUndoScope();
      auto newPos = std::max(1ULL, std::stoull(request->get("order-number"))) - 1;
      auto scope = undoScope.startTransaction("Changed Order Number of Bank: %0", bank->getName(true));
      m_presetManager.setOrderNumber(scope->getTransaction(), uuid, newPos);
    }
  });

  addAction("insert-preset", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto selUuid = request->get("seluuid");
    if(auto bank = m_presetManager.findBankWithPreset(selUuid))
    {
      auto uuid = request->get("uuid");
      auto newName = guessNameBasedOnEditBuffer();

      auto scope = m_presetManager.getUndoScope().startTransaction("Insert preset");
      auto transaction = scope->getTransaction();
      auto desiredPresetPos = bank->getPresetPosition(selUuid) + 1;
      auto newPreset = bank->insertAndLoadPreset(transaction, desiredPresetPos,
                                                 std::make_unique<Preset>(bank, *m_presetManager.getEditBuffer()));

      if(!uuid.empty())
        newPreset->setUuid(transaction, uuid);

      newPreset->setName(transaction, newName);
      bank->selectPreset(transaction, newPreset->getUuid());
      m_presetManager.selectBank(transaction, bank->getUuid());
    }
  });

  addAction("select-preset", [&](std::shared_ptr<NetworkRequest> request) mutable {
    Glib::ustring presetUUID = request->get("uuid");

    if(auto bank = m_presetManager.findBankWithPreset(presetUUID))
    {
      if(auto preset = bank->findPreset(presetUUID))
      {
        UNDO::Scope::tTransactionScopePtr scope;

        bool autoLoad = Application::get().getSettings()->getSetting<DirectLoadSetting>()->get();

        if(autoLoad)
          scope = m_presetManager.getUndoScope().startTransaction(preset->buildUndoTransactionTitle("Load"));
        else
          scope = m_presetManager.getUndoScope().startContinuousTransaction(
              &presetManager, std::chrono::hours(1), preset->buildUndoTransactionTitle("Select"));

        auto transaction = scope->getTransaction();
        m_presetManager.selectBank(transaction, bank->getUuid());
        bank->selectPreset(transaction, presetUUID);
      }
    }
  });

  addAction("delete-preset", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto presetUUID = request->get("uuid");
    auto withBank = request->get("delete-bank");

    if(auto srcBank = m_presetManager.findBankWithPreset(presetUUID))
    {
      if(auto preset = srcBank->findPreset(presetUUID))
      {
        auto scope = m_presetManager.getUndoScope().startTransaction(preset->buildUndoTransactionTitle("Delete"));
        auto transaction = scope->getTransaction();
        srcBank->deletePreset(transaction, presetUUID);

        if(withBank == "true")
          m_presetManager.deleteBank(transaction, srcBank->getUuid());

        m_presetManager.getEditBuffer()->undoableUpdateLoadedPresetInfo(scope->getTransaction());
      }
    }
  });

  addAction("delete-presets", [&](std::shared_ptr<NetworkRequest> request) mutable {
    PerformanceTimer t("delete-presets");
    auto scope = m_presetManager.getUndoScope().startTransaction("Delete Presets");
    auto transaction = scope->getTransaction();

    std::vector<std::string> strs;
    auto csv = request->get("presets");
    auto withBank = request->get("delete-bank");
    boost::split(strs, csv, boost::is_any_of(","));

    for(const auto &presetUUID : strs)
    {
      if(auto srcBank = m_presetManager.findBankWithPreset(presetUUID))
      {
        if(auto preset = srcBank->findPreset(presetUUID))
        {
          srcBank->deletePreset(transaction, presetUUID);
        }
        if(withBank == "true")
        {
          if(srcBank->getNumPresets() == 0)
          {
            m_presetManager.deleteBank(transaction, srcBank->getUuid());
          }
        }
      }
    }

    m_presetManager.getEditBuffer()->undoableUpdateLoadedPresetInfo(scope->getTransaction());
  });

  addAction("load-preset", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto uuid = request->get("uuid");

    if(auto bank = m_presetManager.findBankWithPreset(uuid))
    {
      if(auto preset = bank->findPreset(uuid))
      {
        auto scope = m_presetManager.getUndoScope().startTransaction(preset->buildUndoTransactionTitle("Load"));
        auto transaction = scope->getTransaction();
        m_presetManager.getEditBuffer()->undoableLoad(transaction, preset);
        bank->selectPreset(transaction, preset->getUuid());
      }
    }
  });

  addAction("set-position", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto uuid = request->get("uuid");
    auto x = request->get("x");
    auto y = request->get("y");

    if(auto bank = m_presetManager.findBank(uuid))
    {
      if(bank->getX() != x || bank->getY() != y)
      {
        auto scope = presetManager.getUndoScope().startTransaction("Move preset bank '%0'", bank->getName(true));
        auto transaction = scope->getTransaction();

        bank->setX(transaction, x);
        bank->setY(transaction, y);
      }
    }
  });

  addAction("create-new-bank-from-preset", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto uuid = request->get("preset");
    auto x = request->get("x");
    auto y = request->get("y");

    if(auto bank = m_presetManager.findBankWithPreset(uuid))
    {
      if(auto p = bank->findPreset(uuid))
      {
        auto scope = m_presetManager.getUndoScope().startTransaction("Create new bank");
        auto transaction = scope->getTransaction();
        auto newBank = presetManager.addBank(transaction);
        newBank->setX(transaction, x);
        newBank->setY(transaction, y);
        auto newPreset = newBank->appendPreset(transaction, std::make_unique<Preset>(newBank, *p, true));
        newBank->setName(transaction, "New Bank");
        newBank->selectPreset(transaction, newPreset->getUuid());
        m_presetManager.selectBank(transaction, newBank->getUuid());
      }
    }
  });

  addAction("create-new-bank-from-presets", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto csv = request->get("presets");
    auto x = request->get("x");
    auto y = request->get("y");

    auto scope = m_presetManager.getUndoScope().startTransaction("Create new bank");
    auto transaction = scope->getTransaction();

    auto newBank = presetManager.addBank(transaction);
    newBank->setX(transaction, x);
    newBank->setY(transaction, y);
    newBank->setName(transaction, "New Bank");

    std::vector<std::string> strs;
    boost::split(strs, csv, boost::is_any_of(","));

    for(const auto &uuid : strs)
      if(auto src = presetManager.findPreset(uuid))
        newBank->appendPreset(transaction, std::make_unique<Preset>(newBank, *src, true));

    if(newBank->getNumPresets() > 0)
      newBank->selectPreset(transaction, newBank->getPresetAt(0)->getUuid());

    m_presetManager.selectBank(transaction, newBank->getUuid());
  });

  addAction("drop-bank-on-bank", [&](std::shared_ptr<NetworkRequest> request) mutable {
    Glib::ustring receiverUuid = request->get("receiver");
    Glib::ustring bankUuid = request->get("bank");

    if(auto receiver = m_presetManager.findBank(receiverUuid))
      if(auto bank = m_presetManager.findBank(bankUuid))
        if(bank != receiver)
          insertBank(bank, receiver, receiver->getNumPresets());
  });

  addAction("drop-presets-on-bank", [&](std::shared_ptr<NetworkRequest> request) mutable {
    Glib::ustring bankUUID = request->get("bank");
    Glib::ustring csv = request->get("presets");

    if(auto bank = m_presetManager.findBank(bankUUID))
    {
      UNDO::Scope::tTransactionScopePtr scope = m_presetManager.getUndoScope().startTransaction("Drop Presets on Bank");
      auto transaction = scope->getTransaction();

      std::vector<std::string> strs;
      boost::split(strs, csv, boost::is_any_of(","));

      for(const auto &uuid : strs)
      {
        if(auto src = presetManager.findPreset(uuid))
        {
          bank->appendPreset(transaction, std::make_unique<Preset>(bank, *src, true));

          if(bank == src->getParent())
            bank->deletePreset(transaction, uuid);
        }
      }
    }
  });

  addAction("insert-bank-above", [&](std::shared_ptr<NetworkRequest> request) mutable { insertBank(request, 0); });

  addAction("insert-bank-below", [&](std::shared_ptr<NetworkRequest> request) mutable { insertBank(request, 1); });

  addAction("overwrite-preset-with-bank", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto anchorUuid = request->get("anchor");
    auto bankUuid = request->get("bank");

    if(auto targetBank = m_presetManager.findBankWithPreset(anchorUuid))
      if(auto bank = m_presetManager.findBank(bankUuid))
        if(bank != targetBank)
        {
          auto &undo = m_presetManager.getUndoScope();
          auto srcBankName = bank->getName(true);
          auto tgtBankName = targetBank->getName(true);
          auto scope = undo.startTransaction("Drop bank '%0' into bank '%1'", srcBankName, tgtBankName);
          auto transaction = scope->getTransaction();

          size_t insertPos = targetBank->getPresetPosition(anchorUuid) + 1;
          size_t numPresets = bank->getNumPresets();

          for(size_t i = 0; i < numPresets; i++)
          {
            auto p = std::make_unique<Preset>(targetBank, *bank->getPresetAt(i), true);
            targetBank->insertPreset(transaction, insertPos + i, std::move(p));
          }

          targetBank->deletePreset(scope->getTransaction(), anchorUuid);
        }
  });

  addAction("import-bank", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto xml = request->get("xml");
    auto x = request->get("x");
    auto y = request->get("y");
    auto fileName = request->get("fileName");
    MemoryInStream stream(xml, false);
    importBank(stream, x, y, fileName);
  });

  addAction("set-preset-attribute", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto presetUUID = request->get("uuid");
    auto key = request->get("key");
    auto value = request->get("value");

    if(auto preset = m_presetManager.findPreset(presetUUID))
    {
      auto scope = presetManager.getUndoScope().startTransaction("Set Preset attribute");
      auto transaction = scope->getTransaction();
      preset->setAttribute(transaction, key, value);
      preset->setAutoGeneratedAttributes(transaction);
    }
  });

  addAction("set-bank-attribute", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto bankUUID = request->get("uuid");
    auto key = request->get("key");
    auto value = request->get("value");

    if(auto bank = m_presetManager.findBank(bankUUID))
    {
      auto scope = presetManager.getUndoScope().startTransaction("Set Bank attribute");
      auto transaction = scope->getTransaction();
      bank->setAttribute(transaction, key, value);
      bank->updateLastModifiedTimestamp(transaction);
    }
  });

  addAction("move", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto bankUUID = request->get("bank");
    auto value = request->get("direction");

    if(auto bank = m_presetManager.getSelectedBank())
    {
      auto pos = presetManager.getBankPosition(bankUUID);

      if(value == "LeftByOne")
      {
        auto scope = presetManager.getUndoScope().startTransaction("Move Bank '%0' left", bank->getName(true));
        auto transaction = scope->getTransaction();
        if(pos > 0)
          m_presetManager.setOrderNumber(transaction, bankUUID, pos - 1);
      }
      else if(value == "RightByOne")
      {
        auto scope = presetManager.getUndoScope().startTransaction("Move Bank '%0' right", bank->getName(true));
        auto transaction = scope->getTransaction();
        pos++;
        if(pos < presetManager.getNumBanks())
          m_presetManager.setOrderNumber(transaction, bankUUID, pos);
      }
    }
  });

  addAction("insert-bank-in-cluster", [&](std::shared_ptr<NetworkRequest> request) mutable {
    const auto insertedUUID = request->get("bank-to-insert");
    const auto insertedAtUUID = request->get("bank-inserted-at");
    const auto orientation = request->get("orientation");

    if(auto bankToInsert = m_presetManager.findBank(insertedUUID))
    {
      if(auto bankAtInsertPosition = m_presetManager.findBank(insertedAtUUID))
      {
        insertBankInCluster(bankToInsert, bankAtInsertPosition, orientation);
      }
    }
  });

  addAction("dock-banks", [&](std::shared_ptr<NetworkRequest> request) mutable {
    const auto droppedOntoBankUuid = request->get("droppedOntoBank");
    const auto draggedBankUuid = request->get("draggedBank");
    const auto droppedAt = request->get("droppedAt");
    const auto x = request->get("x");
    const auto y = request->get("y");

    if(auto droppedOntoBank = m_presetManager.findBank(droppedOntoBankUuid))
    {
      if(auto draggedBank = m_presetManager.findBank(draggedBankUuid))
      {
        auto scope = presetManager.getUndoScope().startTransaction(
            "Dock Banks '%0' and '%1'", droppedOntoBank->getName(true), draggedBank->getName(true));
        auto transaction = scope->getTransaction();

        if(droppedAt == "North")
        {
          droppedOntoBank->attachBank(transaction, draggedBank->getUuid(), Bank::AttachmentDirection::top);
        }
        else if(droppedAt == "West")
        {
          droppedOntoBank->attachBank(transaction, draggedBank->getUuid(), Bank::AttachmentDirection::left);
        }
        else if(droppedAt == "South")
        {
          draggedBank->attachBank(transaction, droppedOntoBank->getUuid(), Bank::AttachmentDirection::top);
        }
        else if(droppedAt == "East")
        {
          draggedBank->attachBank(transaction, droppedOntoBank->getUuid(), Bank::AttachmentDirection::left);
        }

        draggedBank->setX(transaction, x);
        draggedBank->setY(transaction, y);

        m_presetManager.sanitizeBankClusterRelations(transaction);
      }
    }
  });

  addAction("undock-bank", [&](std::shared_ptr<NetworkRequest> request) mutable {
    const auto uuid = request->get("uuid");
    const auto x = request->get("x");
    const auto y = request->get("y");

    if(auto bank = m_presetManager.findBank(uuid))
    {
      if(auto attached = m_presetManager.findBank(bank->getAttachedToBankUuid()))
      {
        auto parentBankName = attached->getName(true);
        auto scope = presetManager.getUndoScope().startTransaction("Detached Bank '%0' from '%1'", bank->getName(true),
                                                                   parentBankName);
        auto transaction = scope->getTransaction();

        bank->attachBank(transaction, Uuid::none(), Bank::AttachmentDirection::none);
        bank->setX(transaction, x);
        bank->setY(transaction, y);
      }
    }
  });

  addAction("move-all-banks", [&](std::shared_ptr<NetworkRequest> request) mutable {
    auto x = atof(request->get("x").c_str());
    auto y = atof(request->get("y").c_str());

    UNDO::Scope::tTransactionScopePtr scope = m_presetManager.getUndoScope().startTransaction("Move all Banks");
    UNDO::Transaction *transaction = scope->getTransaction();

    for(auto bank : m_presetManager.getBanks())
    {
      bank->setX(transaction, to_string(std::stoi(bank->getX()) + x));
      bank->setY(transaction, to_string(std::stoi(bank->getY()) + y));
    }
  });

  addAction("sort-bank-numbers", [&](auto request) mutable { ClusterEnforcement::sortBankNumbers(); });
}

BankActions::~BankActions()
{
}

void BankActions::dropPresets(UNDO::Transaction *transaction, const Glib::ustring &anchorUUID, int offset,
                              const Glib::ustring &csv)
{
  std::vector<std::string> strs;
  boost::split(strs, csv, boost::is_any_of(","));

  if(auto anchor = m_presetManager.findPreset(anchorUUID))
  {
    auto bank = static_cast<Bank *>(anchor->getParent());
    auto anchorPos = static_cast<int>(bank->getPresetPosition(anchorUUID));
    auto pos = std::max(anchorPos + offset, 0);

    for(const auto &presetUUID : strs)
    {
      if(auto src = m_presetManager.findPreset(presetUUID))
      {
        if(src->getParent() == anchor->getParent())
        {
          bank->movePreset(transaction, src, anchor);
        }
        else
        {
          bank->insertPreset(transaction, size_t(pos), std::make_unique<Preset>(bank, *src, true));
          pos++;
        }
      }
    }
  }
}

void BankActions::insertBank(std::shared_ptr<NetworkRequest> request, size_t offset)
{
  Glib::ustring anchorUuid = request->get("anchor");
  Glib::ustring bankUuid = request->get("bank");

  if(auto preset = m_presetManager.findPreset(anchorUuid))
    if(auto targetBank = static_cast<Bank *>(preset->getParent()))
      if(auto bank = m_presetManager.findBank(bankUuid))
        if(bank != targetBank)
        {
          auto anchorPos = targetBank->getPresetPosition(anchorUuid);
          insertBank(bank, targetBank, anchorPos + offset);
        }
}

void BankActions::insertBank(Bank *bank, Bank *targetBank, size_t insertPos)
{
  auto scope = m_presetManager.getUndoScope().startTransaction("Drop bank '%0' into bank '%1'", bank->getName(true),
                                                               targetBank->getName(true));
  auto transaction = scope->getTransaction();
  size_t i = 0;

  bank->forEachPreset([&](auto p) {
    targetBank->insertPreset(transaction, insertPos + i, std::make_unique<Preset>(targetBank, *p, true));
    i++;
  });
}

bool BankActions::handleRequest(const Glib::ustring &path, std::shared_ptr<NetworkRequest> request)
{
  if(super::handleRequest(path, request))
    return true;

  if(path.find("/presets/banks/download-bank/") == 0)
  {
    PerformanceTimer timer(__PRETTY_FUNCTION__);

    if(auto httpRequest = std::dynamic_pointer_cast<HTTPRequest>(request))
    {
      Glib::ustring uuid = request->get("uuid");

      if(auto bank = m_presetManager.findBank(uuid))
      {
        auto stream = request->createStream("text/xml", false);
        httpRequest->setHeader("Content-Disposition", "attachment; filename=\"" + bank->getName(true) + ".xml\"");
        XmlWriter writer(stream);
        PresetBankSerializer serializer(bank);
        serializer.write(writer, VersionAttribute::get());

        auto scope = UNDO::Scope::startTrashTransaction();
        auto transaction = scope->getTransaction();
        bank->setAttribute(transaction, "Name of Export File", "(via Browser)");
        bank->setAttribute(transaction, "Date of Export File", TimeTools::getAdjustedIso());
      }

      return true;
    }
  }

  if(path.find("/presets/banks/download-preset/") == 0)
  {
    PerformanceTimer timer(__PRETTY_FUNCTION__);

    if(auto httpRequest = std::dynamic_pointer_cast<HTTPRequest>(request))
    {
      auto stream = request->createStream("text/xml", false);
      XmlWriter writer(stream);

      Glib::ustring uuid = request->get("uuid");

      Preset *preset = nullptr;
      auto ebAsPreset = std::make_unique<Preset>(&m_presetManager, *m_presetManager.getEditBuffer());

      if(uuid.empty())
      {
        preset = ebAsPreset.get();
      }
      else if(auto p = m_presetManager.findPreset(uuid))
      {
        preset = p;
      }
      else
      {
        DebugLevel::warning("Could not download Preset!");
        return false;
      }

      PresetSerializer serializer(preset);
      httpRequest->setHeader("Content-Disposition", "attachment; filename=\"" + preset->getName() + ".xml\"");
      serializer.write(writer, VersionAttribute::get());

      return true;
    }
  }

  return false;
}

Bank *BankActions::importBank(InStream &stream, Glib::ustring x, Glib::ustring y, const Glib::ustring &fileName)
{
  UNDO::Scope::tTransactionScopePtr scope = m_presetManager.getUndoScope().startTransaction("Import new Bank");
  auto transaction = scope->getTransaction();
  return importBank(transaction, stream, x, y, fileName);
}

Bank *BankActions::importBank(UNDO::Transaction *transaction, InStream &stream, Glib::ustring x, Glib::ustring y,
                              const Glib::ustring &fileName)
{
  auto autoLoadOff = Application::get().getSettings()->getSetting<DirectLoadSetting>()->scopedOverlay(
      BooleanSettings::BOOLEAN_SETTING_FALSE);
  
  auto newBank = m_presetManager.addBank(transaction, std::make_unique<Bank>(&m_presetManager));

  XmlReader reader(stream, transaction);
  reader.read<PresetBankSerializer>(newBank, true);

  newBank->setAttachedToBank(transaction, Uuid::none());
  newBank->setAttachedDirection(transaction, to_string(Bank::AttachmentDirection::none));

  if(x.empty() || y.empty())
  {
    auto pos = m_presetManager.calcDefaultBankPositionFor(newBank);
    x = to_string(pos.first);
    y = to_string(pos.second);
  }

  newBank->setX(transaction, x);
  newBank->setY(transaction, y);

  newBank->ensurePresetSelection(transaction);
  newBank->setAttribute(transaction, "Name of Import File", fileName);
  newBank->setAttribute(transaction, "Date of Import File", TimeTools::getAdjustedIso());
  newBank->setAttribute(transaction, "Name of Export File", "");
  newBank->setAttribute(transaction, "Date of Export File", "");

  m_presetManager.ensureBankSelection(transaction);

  Application::get().getHWUI()->setFocusAndMode(FocusAndMode(UIFocus::Presets, UIMode::Select));

  return newBank;
}

Glib::ustring BankActions::guessNameBasedOnEditBuffer() const
{
  auto eb = m_presetManager.getEditBuffer();
  auto ebName = eb->getName();

  if(eb->isModified())
    return m_presetManager.createPresetNameBasedOn(ebName);

  return ebName;
}

void BankActions::insertBankInCluster(Bank *bankToInsert, Bank *bankAtInsert,
                                      const Glib::ustring directionSeenFromBankInCluster)
{
  auto scope
      = m_presetManager.getUndoScope().startTransaction("Insert Bank %0 into Cluster", bankToInsert->getName(true));
  auto transaction = scope->getTransaction();

  if(auto slaveBottom = bankToInsert->getSlaveBottom())
    slaveBottom->attachBank(transaction, Uuid::none(), Bank::AttachmentDirection::none);

  if(auto slaveRight = bankToInsert->getSlaveRight())
    slaveRight->attachBank(transaction, Uuid::none(), Bank::AttachmentDirection::none);

  if(directionSeenFromBankInCluster == "North")
  {
    if(auto topMaster = bankAtInsert->getMasterTop())
    {
      bankToInsert->attachBank(transaction, topMaster->getUuid(), Bank::AttachmentDirection::top);
    }
    bankAtInsert->attachBank(transaction, bankToInsert->getUuid(), Bank::AttachmentDirection::top);
  }
  else if(directionSeenFromBankInCluster == "South")
  {
    if(auto bottomSlave = bankAtInsert->getSlaveBottom())
    {
      bottomSlave->attachBank(transaction, bankToInsert->getUuid(), Bank::AttachmentDirection::top);
    }
    bankToInsert->attachBank(transaction, bankAtInsert->getUuid(), Bank::AttachmentDirection::top);
  }
  else if(directionSeenFromBankInCluster == "East")
  {
    if(auto rightSlave = bankAtInsert->getSlaveRight())
    {
      rightSlave->attachBank(transaction, bankToInsert->getUuid(), Bank::AttachmentDirection::left);
    }
    bankToInsert->attachBank(transaction, bankAtInsert->getUuid(), Bank::AttachmentDirection::left);
  }
  else if(directionSeenFromBankInCluster == "West")
  {
    if(auto leftMaster = bankAtInsert->getMasterLeft())
    {
      bankToInsert->attachBank(transaction, leftMaster->getUuid(), Bank::AttachmentDirection::left);
    }
    bankAtInsert->attachBank(transaction, bankToInsert->getUuid(), Bank::AttachmentDirection::left);
  }
}
