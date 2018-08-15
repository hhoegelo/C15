#pragma once

#include <libundo/undo/Scope.h>
#include "PresetManager.h"

class PresetBank;

class BankSorter {
public:
    typedef PresetManager::tBankPtr tPresetBankPtr;
    typedef vector<tPresetBankPtr> BankVector;
    static void sort();
private:
    static shared_ptr<PresetManager> getPM();
    static BankVector getClusterMastersSortedByX();
    static BankVector getFreeBanksSortedByX();
    static BankVector concatVectorsAccordingToRule(BankVector clusters, const BankVector &freeBanks);
    static BankVector getClusterVectorsFromClusterMasters(BankVector clusterMasters);
    static bool compareByXPosition(const tPresetBankPtr &lhs, const tPresetBankPtr &rhs);
    static bool isValidMasterBank(PresetBank* bank, PresetBank* master);
    static bool doesNotContain(BankVector &clusterMasters, tPresetBankPtr master);
};
