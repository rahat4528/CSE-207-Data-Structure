#include "MiniTest.h"
#include "KeyManager.h"
#include "EventLog.h"
#include "StorageEngine.h"
#include <filesystem>
#include <fstream>
#include <string>

using namespace ttkv;

TEST_CASE(TestPersistenceRestartAndExactRecovery) {
    std::string walPath = "test_restart.wal";
    std::filesystem::remove(walPath);

    KeyManager kmWrite;
    EventLog elWrite;
    StorageEngine storage(walPath);

    ASSERT_TRUE(storage.open().isOk());

    // Write sequence of 5 records across 2 keys
    auto v1 = kmWrite.prepareWrite("user:alice", "admin", 1000ULL).value();
    kmWrite.applyRecord(v1);
    elWrite.appendEvent(v1);
    ASSERT_TRUE(storage.appendRecord(v1).isOk());

    auto v2 = kmWrite.prepareWrite("user:bob", "guest", 1100ULL).value();
    kmWrite.applyRecord(v2);
    elWrite.appendEvent(v2);
    ASSERT_TRUE(storage.appendRecord(v2).isOk());

    auto v3 = kmWrite.prepareWrite("user:alice", "superadmin", 1200ULL).value();
    kmWrite.applyRecord(v3);
    elWrite.appendEvent(v3);
    ASSERT_TRUE(storage.appendRecord(v3).isOk());

    auto v4 = kmWrite.prepareDelete("user:bob", 1300ULL).value();
    kmWrite.applyRecord(v4);
    elWrite.appendEvent(v4);
    ASSERT_TRUE(storage.appendRecord(v4).isOk());

    auto v5 = kmWrite.prepareWrite("user:bob", "reborn_user", 1400ULL).value();
    kmWrite.applyRecord(v5);
    elWrite.appendEvent(v5);
    ASSERT_TRUE(storage.appendRecord(v5).isOk());

    storage.close();

    // Now simulate full process restart: new KeyManager, new EventLog, new StorageEngine
    KeyManager kmRecovered;
    EventLog elRecovered;
    StorageEngine storageRecover(walPath);

    auto recRes = storageRecover.recover(kmRecovered, elRecovered, false);
    ASSERT_TRUE(recRes.isOk());
    ASSERT_EQ(recRes.value(), 5u);

    // Verify state identity
    ASSERT_EQ(kmRecovered.keyCount(true), 2u);
    ASSERT_EQ(kmRecovered.totalVersionCount(), 5u);

    const KeyRecord* alice = kmRecovered.getKeyRecord("user:alice");
    ASSERT_TRUE(alice != nullptr);
    ASSERT_EQ(alice->versions.size(), 2u);
    ASSERT_EQ(alice->versions[0].value(), std::string("admin"));
    ASSERT_EQ(alice->versions[1].value(), std::string("superadmin"));

    const KeyRecord* bob = kmRecovered.getKeyRecord("user:bob");
    ASSERT_TRUE(bob != nullptr);
    ASSERT_EQ(bob->versions.size(), 3u);
    ASSERT_EQ(bob->versions[0].value(), std::string("guest"));
    ASSERT_TRUE(bob->versions[1].isDeleted());
    ASSERT_EQ(bob->versions[2].value(), std::string("reborn_user"));

    storageRecover.close();
    std::filesystem::remove(walPath);
}

TEST_CASE(TestDetectCorruptedRecordFlippedByte) {
    std::string walPath = "test_corrupt.wal";
    std::filesystem::remove(walPath);

    KeyManager kmWrite;
    EventLog elWrite;
    StorageEngine storage(walPath);
    storage.open();

    auto v1 = kmWrite.prepareWrite("key1", "val1", 1000ULL).value();
    storage.appendRecord(v1);
    auto v2 = kmWrite.prepareWrite("key2", "val2", 2000ULL).value();
    storage.appendRecord(v2);
    storage.close();

    // Flip a byte in the middle of key1 payload
    {
        std::fstream file(walPath, std::ios::in | std::ios::out | std::ios::binary);
        file.seekp(30, std::ios::beg); // Inside payload
        char byteVal = 0;
        file.read(&byteVal, 1);
        byteVal ^= 0x55; // Bit flip
        file.seekp(30, std::ios::beg);
        file.write(&byteVal, 1);
        file.flush();
    }

    // Attempt recovery: must detect corruption and reject with CRC mismatch
    KeyManager kmTest;
    EventLog elTest;
    StorageEngine storageTest(walPath);
    auto recRes = storageTest.recover(kmTest, elTest, false);

    ASSERT_TRUE(recRes.isError());
    ASSERT_EQ(recRes.error(), ErrorCode::ERR_CHECKSUM_MISMATCH);

    storageTest.close();
    std::filesystem::remove(walPath);
}

TEST_CASE(TestTornTailVersusMidFileCorruption) {
    std::string walPath = "test_torn.wal";
    std::filesystem::remove(walPath);

    KeyManager kmWrite;
    EventLog elWrite;
    StorageEngine storage(walPath);
    storage.open();

    auto v1 = kmWrite.prepareWrite("k1", "v1", 1000ULL).value();
    kmWrite.applyRecord(v1);
    storage.appendRecord(v1);
    auto v2 = kmWrite.prepareWrite("k2", "v2", 2000ULL).value();
    kmWrite.applyRecord(v2);
    storage.appendRecord(v2);
    storage.close();

    // Simulate a crash that left a torn write at the end of the file (e.g. 5 extra garbage bytes appended)
    {
        std::ofstream out(walPath, std::ios::binary | std::ios::app);
        const uint8_t garbage[] = {0x12, 0x34, 0x56, 0x78, 0x90};
        out.write(reinterpret_cast<const char*>(garbage), sizeof(garbage));
        out.flush();
    }

    // 1. Without recoveryMode: reports torn record error, does not alter file
    KeyManager km1;
    EventLog el1;
    StorageEngine s1(walPath);
    auto errRes = s1.recover(km1, el1, false);
    ASSERT_TRUE(errRes.isError());

    // 2. With recoveryMode: safely truncates torn tail and recovers the 2 valid records
    KeyManager km2;
    EventLog el2;
    StorageEngine s2(walPath);
    auto okRes = s2.recover(km2, el2, true);
    ASSERT_TRUE(okRes.isOk());
    ASSERT_EQ(okRes.value(), 2u);
    ASSERT_TRUE(km2.hasKey("k1") && km2.hasKey("k2"));

    s2.close();
    std::filesystem::remove(walPath);
}

TEST_CASE(TestAtomicWriteFile) {
    std::string target = "test_atomic.txt";
    std::filesystem::remove(target);

    std::string content = "TimeTravelStorage: Atomic Content Test";
    auto res = StorageEngine::atomicWriteFile(target, content);
    ASSERT_TRUE(res.isOk());
    ASSERT_TRUE(std::filesystem::exists(target));

    std::ifstream in(target);
    std::string readBack;
    std::getline(in, readBack);
    ASSERT_EQ(readBack, content);
    in.close();

    std::filesystem::remove(target);
}
