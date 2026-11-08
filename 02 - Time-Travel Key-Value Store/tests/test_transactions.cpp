#include "MiniTest.h"
#include "KeyManager.h"
#include "EventLog.h"
#include "StorageEngine.h"
#include "TransactionManager.h"
#include "TemporalSearch.h"
#include <string>
#include <filesystem>

using namespace ttkv;

TEST_CASE(TestTransactionAtomicCommitAndReadYourOwnWrites) {
    KeyManager km;
    EventLog el;
    TransactionManager tm;

    // Committed initial state: accountA=100, accountB=50
    km.applyRecord(km.prepareWrite("accountA", "100", 1000ULL).value());
    km.applyRecord(km.prepareWrite("accountB", "50", 1000ULL).value());

    // Begin atomic transfer transaction
    auto txIdRes = tm.begin(2000ULL, "bank_service");
    ASSERT_TRUE(txIdRes.isOk());
    ASSERT_TRUE(tm.isInTransaction());

    // Stage updates: accountA=70, accountB=80
    ASSERT_TRUE(tm.stageWrite("accountA", "70", 0, "Transfer debit").isOk());
    ASSERT_TRUE(tm.stageWrite("accountB", "80", 0, "Transfer credit").isOk());

    // Read-your-own-writes inside transaction: see staged values
    auto stagedA = tm.readStaged("accountA");
    ASSERT_TRUE(stagedA.has_value());
    ASSERT_EQ(stagedA->value, std::string("70"));

    // Outside view (km): still sees uncommitted values (100 and 50)
    ASSERT_EQ(km.getKeyRecord("accountA")->versions.back().value(), std::string("100"));
    ASSERT_EQ(km.getKeyRecord("accountB")->versions.back().value(), std::string("50"));

    // Commit transaction
    auto commitRes = tm.commit(km, el, nullptr, 2000ULL);
    ASSERT_TRUE(commitRes.isOk());
    ASSERT_EQ(commitRes.value(), 2u);
    ASSERT_FALSE(tm.isInTransaction());

    // Now committed state reflects the transfer
    ASSERT_EQ(km.getKeyRecord("accountA")->versions.back().value(), std::string("70"));
    ASSERT_EQ(km.getKeyRecord("accountB")->versions.back().value(), std::string("80"));
    ASSERT_EQ(km.getKeyRecord("accountA")->latestVersionNumber, 2u);
    ASSERT_EQ(km.getKeyRecord("accountB")->latestVersionNumber, 2u);

    // Verify transaction history query
    const auto& hist = tm.transactionHistory();
    ASSERT_EQ(hist.size(), 1u);
    ASSERT_TRUE(hist[0].committed);
    ASSERT_EQ(hist[0].operationCount, 2u);
    ASSERT_EQ(hist[0].author, std::string("bank_service"));
}

TEST_CASE(TestTransactionAbortUnwindsRollbackStack) {
    KeyManager km;
    EventLog el;
    TransactionManager tm;

    km.applyRecord(km.prepareWrite("balance", "500", 1000ULL).value());

    tm.begin(2000ULL, "user1");
    tm.stageWrite("balance", "200");
    tm.stageDelete("balance");
    ASSERT_TRUE(tm.readStaged("balance")->isDeleted);

    // Abort transaction
    ASSERT_TRUE(tm.abort().isOk());
    ASSERT_FALSE(tm.isInTransaction());

    // Staged overlay is gone, store state is untouched
    ASSERT_EQ(km.getKeyRecord("balance")->versions.back().value(), std::string("500"));
    ASSERT_FALSE(km.isDeleted("balance"));

    const auto& hist = tm.transactionHistory();
    ASSERT_EQ(hist.size(), 1u);
    ASSERT_FALSE(hist[0].committed);
}

TEST_CASE(TestTransactionValidationFailureNoPartialCommit) {
    KeyManager km;
    EventLog el;
    TransactionManager tm;

    km.applyRecord(km.prepareWrite("validKey", "initVal", 1000ULL).value());

    tm.begin(2000ULL, "admin");
    tm.stageWrite("validKey", "newVal");
    // Stage an invalid operation (deleting a non-existent key)
    tm.stageDelete("non_existent_key_xyz");

    // Commit must fail and roll back everything: validKey must NOT be updated!
    auto commitRes = tm.commit(km, el, nullptr, 2000ULL);
    ASSERT_TRUE(commitRes.isError());
    ASSERT_EQ(commitRes.error(), ErrorCode::ERR_KEY_UNKNOWN);
    ASSERT_FALSE(tm.isInTransaction());

    // Verify validKey was not modified (NO PARTIAL COMMIT)
    ASSERT_EQ(km.getKeyRecord("validKey")->versions.back().value(), std::string("initVal"));
    ASSERT_EQ(km.getKeyRecord("validKey")->latestVersionNumber, 1u);
}
