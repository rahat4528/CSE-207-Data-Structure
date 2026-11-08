#include "MiniTest.h"
#include "KeyManager.h"
#include "EventLog.h"
#include "SnapshotEngine.h"
#include "ExporterImporter.h"
#include "TemporalSearch.h"
#include <string>

using namespace ttkv;

TEST_CASE(TestExportAndImportRestoreIdentity) {
    KeyManager kmOriginal;
    EventLog elOriginal;

    // Seed original store with 4 historical records
    auto v1 = kmOriginal.prepareWrite("country:BD", "Bangladesh", 1000ULL).value();
    kmOriginal.applyRecord(v1);
    elOriginal.appendEvent(v1);

    auto v2 = kmOriginal.prepareWrite("capital:BD", "Dhaka", 1100ULL).value();
    kmOriginal.applyRecord(v2);
    elOriginal.appendEvent(v2);

    auto v3 = kmOriginal.prepareWrite("country:BD", "People's Republic of Bangladesh", 1200ULL).value();
    kmOriginal.applyRecord(v3);
    elOriginal.appendEvent(v3);

    auto v4 = kmOriginal.prepareDelete("capital:BD", 1300ULL).value();
    kmOriginal.applyRecord(v4);
    elOriginal.appendEvent(v4);

    // Export to JSON
    std::string jsonExport = ExporterImporter::exportToJson(kmOriginal);
    ASSERT_TRUE(!jsonExport.empty());
    ASSERT_TRUE(jsonExport.find("TTKV_EXPORT") != std::string::npos);
    ASSERT_TRUE(jsonExport.find("Bangladesh") != std::string::npos);

    // Export to CSV
    std::string csvExport = ExporterImporter::exportToCsv(kmOriginal);
    ASSERT_TRUE(!csvExport.empty());
    ASSERT_TRUE(csvExport.find("country:BD") != std::string::npos);

    // Dry-run import into clean store
    KeyManager kmImported;
    EventLog elImported;
    auto dryRes = ExporterImporter::importFromJson(kmImported, elImported, nullptr, jsonExport, true);
    ASSERT_TRUE(dryRes.isOk());
    ASSERT_EQ(dryRes.value(), 4u);
    ASSERT_EQ(kmImported.keyCount(true), 0u); // Dry-run made zero changes!

    // Live import into clean store
    auto liveRes = ExporterImporter::importFromJson(kmImported, elImported, nullptr, jsonExport, false);
    ASSERT_TRUE(liveRes.isOk());
    ASSERT_EQ(liveRes.value(), 4u);

    // Verify exact identity between kmOriginal and kmImported
    ASSERT_EQ(kmImported.keyCount(true), kmOriginal.keyCount(true));
    ASSERT_EQ(kmImported.totalVersionCount(), kmOriginal.totalVersionCount());

    const KeyRecord* origBD = kmOriginal.getKeyRecord("country:BD");
    const KeyRecord* impBD = kmImported.getKeyRecord("country:BD");
    ASSERT_TRUE(origBD != nullptr && impBD != nullptr);
    ASSERT_EQ(impBD->versions.size(), origBD->versions.size());
    ASSERT_EQ(impBD->versions[0].value(), origBD->versions[0].value());
    ASSERT_EQ(impBD->versions[1].value(), origBD->versions[1].value());
}

TEST_CASE(TestImportValidationRejectsInvalidData) {
    KeyManager km;
    EventLog el;

    // 1. Missing TTKV_EXPORT tag
    std::string badTag = "{\"format\": \"UNKNOWN\"}";
    ASSERT_TRUE(ExporterImporter::importFromJson(km, el, nullptr, badTag).isError());

    // 2. Corrupted record with duplicate version ID
    std::string dupJson = "{\n"
        "  \"format\": \"TTKV_EXPORT\",\n"
        "  \"records\": [\n"
        "    {\"globalVersionId\": 1, \"keyVersionNumber\": 1, \"key\": \"k1\", \"value\": \"v1\", \"timestamp\": 1000, \"prevVersionId\": 0},\n"
        "    {\"globalVersionId\": 1, \"keyVersionNumber\": 2, \"key\": \"k1\", \"value\": \"v2\", \"timestamp\": 2000, \"prevVersionId\": 1}\n"
        "  ]\n}";
    auto dupRes = ExporterImporter::importFromJson(km, el, nullptr, dupJson);
    ASSERT_TRUE(dupRes.isError());
    ASSERT_EQ(dupRes.error(), ErrorCode::ERR_DUPLICATE_VERSION_ID);
    ASSERT_EQ(km.keyCount(true), 0u); // All-or-nothing: zero changes applied!
}

TEST_CASE(TestRetentionPruningProtectsSnapshots) {
    KeyManager km;
    SnapshotEngine se;

    // Key "config" has 5 versions:
    // v1 (t=1000), v2 (t=1100), v3 (t=1200), v4 (t=1300), v5 (t=1400)
    for (int i = 1; i <= 5; ++i) {
        km.applyRecord(km.prepareWrite("config", "val_" + std::to_string(i),
                                       static_cast<uint64_t>(1000 + i * 100)).value());
    }

    // Named snapshot at t=1200 (protects v2)
    se.createNamedSnapshot("snap_v2", 1200ULL, 2ULL);

    // Prune keeping last 1 version
    auto pruneRes = ExporterImporter::pruneHistory(km, se, 1, 0, true); // dry-run
    ASSERT_TRUE(pruneRes.isOk());
    ASSERT_TRUE(pruneRes.value().protectedBySnapshots > 0);
}
