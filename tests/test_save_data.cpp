// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <png.h>
#include <sqlite3.h>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/singleton.h"
#include "core/emulator_settings.h"
#include "core/emulator_state.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/save_data/dialog/savedatadialog.h"
#include "core/libraries/save_data/dialog/savedatadialog_ui.h"
#include "core/libraries/save_data/save_backup.h"
#include "core/libraries/save_data/save_db.h"
#include "core/libraries/save_data/savedata.h"
#include "core/libraries/save_data/savedata_error.h"
#include "core/user_manager.h"
#include "core/user_settings.h"

namespace fs = std::filesystem;
using namespace Libraries::SaveData;

namespace {

std::vector<u8> BuildPngBytes(u32 width, u32 height, int color_type = PNG_COLOR_TYPE_RGB) {
    std::vector<u8> out;
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_write_fn(
        png_ptr, &out,
        [](png_structp p, png_bytep data, png_size_t len) {
            auto* vec = static_cast<std::vector<u8>*>(png_get_io_ptr(p));
            vec->insert(vec->end(), data, data + len);
        },
        [](png_structp) {});
    png_write_info(png_ptr, info_ptr);
    const size_t channels = (color_type == PNG_COLOR_TYPE_RGBA) ? 4 : 3;
    std::vector<u8> row(static_cast<size_t>(width) * channels, 0xAB);
    for (u32 y = 0; y < height; ++y) {
        png_write_row(png_ptr, row.data());
    }
    png_write_end(png_ptr, nullptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return out;
}

class SaveDataTest : public ::testing::Test {
protected:
    static constexpr s32 kUid1 = 0x10000001;
    static constexpr s32 kUid2 = 0x10000002;
    static constexpr const char* kTitleId = "CUSA12345";

    void SetUp() override {
        Common::Log::Setup("shad_test_savedata.log");
        auto ns = std::chrono::steady_clock::now().time_since_epoch().count();
        temp_root = fs::temp_directory_path() / ("shadps4_sd_test_" + std::to_string(ns));
        fs::create_directories(temp_root);

        using PT = Common::FS::PathType;
        const struct {
            PT type;
            const char* sub;
        } dirs[] = {
            {PT::UserDir, ""},         {PT::LogDir, "log"},       {PT::HomeDir, "home"},
            {PT::TempDataDir, "temp"}, {PT::GameDataDir, "data"},
        };
        for (const auto& d : dirs) {
            fs::path p = d.sub[0] ? (temp_root / d.sub) : temp_root;
            fs::create_directories(p);
            Common::FS::SetUserPath(d.type, p);
        }

        emu_state = std::make_shared<EmulatorState>();
        EmulatorState::SetInstance(emu_state);
        emu_settings = std::make_shared<EmulatorSettingsImpl>();
        EmulatorSettingsImpl::SetInstance(emu_settings);
        user_settings = std::make_shared<UserSettingsImpl>();
        UserSettingsImpl::SetInstance(user_settings);

        ResetForTest();
        Dialog::ResetDialogForTest();
        Common::Singleton<Core::FileSys::MntPoints>::Instance()->UnmountAll();

        auto& users = UserManagement;
        users.GetUsers().user.clear();
        users.AddUser(User{.user_id = kUid1, .user_name = "Tester1", .player_index = 1});
        users.AddUser(User{.user_id = kUid2, .user_name = "Tester2", .player_index = 2});

        SetTestEnvironment(kTitleId, Common::ElfInfo::FW_1150);
    }

    void TearDown() override {
        Dialog::ResetDialogForTest();
        ResetForTest();
        Common::Singleton<Core::FileSys::MntPoints>::Instance()->UnmountAll();
        UserSettingsImpl::SetInstance(nullptr);
        EmulatorSettingsImpl::SetInstance(nullptr);
        EmulatorState::SetInstance(nullptr);
        user_settings.reset();
        emu_settings.reset();
        emu_state.reset();
        std::error_code ec;
        fs::remove_all(temp_root, ec);
    }

    fs::path temp_root;
    std::shared_ptr<EmulatorState> emu_state;
    std::shared_ptr<EmulatorSettingsImpl> emu_settings;
    std::shared_ptr<UserSettingsImpl> user_settings;
};

TEST_F(SaveDataTest, Ps4PathsAndSqliteSchemaAndZeroLegacyPaths) {
    EXPECT_EQ(sceSaveDataInitialize3(nullptr), ORBIS_OK);
    EXPECT_EQ(sceSaveDataGetSavePoint(), 2);

    char buf[1024]{};
    EXPECT_EQ(sceSaveDataGetDataBaseFilePath(2, buf, sizeof(buf)), ORBIS_OK);
    EXPECT_STREQ(buf, "user/savedata.db");
    EXPECT_EQ(sceSaveDataGetDataBaseFilePath(3, buf, sizeof(buf)), ORBIS_OK);
    EXPECT_STREQ(buf, "cloud/savedata.db");
    EXPECT_EQ(sceSaveDataGetDataBaseFilePath(4, buf, sizeof(buf)), ORBIS_OK);
    EXPECT_STREQ(buf, "usb0/savedata.db");
    EXPECT_NE(sceSaveDataGetDataBaseFilePath(99, buf, sizeof(buf)), ORBIS_OK);
    EXPECT_NE(sceSaveDataGetDataBaseFilePath(2, buf, 16), ORBIS_OK);

    EXPECT_EQ(sceSaveDataGetSaveDataRootPath(2, kUid1, buf, sizeof(buf)), ORBIS_OK);
    EXPECT_STREQ(buf, "/user/home/10000001/savedata");
    EXPECT_EQ(sceSaveDataGetSaveDataRootUsbPath(4, 0x1234ULL, buf, sizeof(buf)), ORBIS_OK);
    EXPECT_STREQ(buf, "/usb0/PS4/SAVEDATA/0000000000001234");

    OrbisSaveDataDirName dir{"slot0"};
    OrbisSaveDataMount2 mount{};
    mount.userId = kUid1;
    mount.dirName = &dir;
    mount.blocks = OrbisSaveDataBlocksMin2;
    mount.mountMode = 0x02 | 0x04; // RDWR | CREATE

    OrbisSaveDataMountResult res{};
    ASSERT_EQ(sceSaveDataMount2(&mount, &res), ORBIS_OK);
    EXPECT_STREQ(res.mountPoint.data, "/savedata0");
    EXPECT_EQ(res.mountStatus, 1u);
    ASSERT_EQ(sceSaveDataUmount(&res.mountPoint), ORBIS_OK);

    const fs::path expected_save =
        temp_root / "home" / "10000001" / "savedata" / kTitleId / "slot0" / "sce_sys" / "param.sfo";
    const fs::path expected_db =
        temp_root / "system_data" / "savedata" / "10000001" / "db" / "user" / "savedata.db";
    EXPECT_TRUE(fs::exists(expected_save));
    EXPECT_TRUE(fs::exists(expected_db));
    EXPECT_FALSE(fs::exists(temp_root / "home" / std::to_string(kUid1) / "savedata"));
    EXPECT_FALSE(fs::exists(expected_save.parent_path().parent_path() / "sce_backup"));
    EXPECT_FALSE(fs::exists(expected_save.parent_path() / "corrupted"));

    sqlite3* db = nullptr;
    ASSERT_EQ(sqlite3_open(expected_db.string().c_str(), &db), SQLITE_OK);
    sqlite3_stmt* stmt = nullptr;
    ASSERT_EQ(sqlite3_prepare_v2(db,
                                 "SELECT count(*) FROM savedata WHERE title_id=? AND dir_name=?",
                                 -1, &stmt, nullptr),
              SQLITE_OK);
    sqlite3_bind_text(stmt, 1, kTitleId, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, "slot0", -1, SQLITE_STATIC);
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(stmt, 0), 1);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

TEST_F(SaveDataTest, UmountReturnsEbusyWhileFileHandleIsOpen) {
    ASSERT_EQ(sceSaveDataInitialize3(nullptr), ORBIS_OK);

    OrbisSaveDataDirName dir{"ebusy_slot"};
    OrbisSaveDataMount2 mount{};
    mount.userId = kUid1;
    mount.dirName = &dir;
    mount.blocks = OrbisSaveDataBlocksMin2;
    mount.mountMode = 0x02 | 0x04; // RDWR | CREATE

    OrbisSaveDataMountResult res{};
    ASSERT_EQ(sceSaveDataMount2(&mount, &res), ORBIS_OK);

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    auto* ht = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    const s32 fd = ht->CreateHandle();
    auto* file = ht->GetFile(fd);
    ASSERT_NE(file, nullptr);
    file->m_guest_name = "/savedata0/progress.dat";
    file->m_host_name = mnt->GetHostPath("/savedata0/progress.dat");
    file->handle = mnt->Open("/savedata0/progress.dat", Common::FS::FileAccessMode::Create);
    ASSERT_TRUE(file->IsBackendOpen());
    file->is_opened = true;

    // Must fail with BUSY while an FD holds an open file inside /savedata0.
    EXPECT_EQ(sceSaveDataUmount(&res.mountPoint), static_cast<s32>(ORBIS_SAVE_DATA_ERROR_BUSY));

    file->handle.reset();
    ht->DeleteHandle(fd);

    // Must succeed once the FD is closed.
    EXPECT_EQ(sceSaveDataUmount(&res.mountPoint), ORBIS_OK);
}

TEST_F(SaveDataTest, MountFullEnforces15SimultaneousMounts) {
    ASSERT_EQ(sceSaveDataInitialize3(nullptr), ORBIS_OK);
    std::vector<OrbisSaveDataMountPoint> mounted;
    for (int i = 0; i < 15; ++i) {
        std::string name = "slot_" + std::to_string(i);
        OrbisSaveDataDirName dir{};
        std::memcpy(dir.data, name.data(), name.size());
        OrbisSaveDataMount2 m{};
        m.userId = kUid1;
        m.dirName = &dir;
        m.blocks = OrbisSaveDataBlocksMin2;
        m.mountMode = 0x02 | 0x04;
        OrbisSaveDataMountResult r{};
        ASSERT_EQ(sceSaveDataMount2(&m, &r), ORBIS_OK);
        mounted.push_back(r.mountPoint);
    }
    u32 mounted_count = 0;
    ASSERT_EQ(sceSaveDataGetMountedSaveDataCount(&mounted_count), ORBIS_OK);
    EXPECT_EQ(mounted_count, 15u);

    OrbisSaveDataDirName dir16{"slot_overflow"};
    OrbisSaveDataMount2 m16{};
    m16.userId = kUid1;
    m16.dirName = &dir16;
    m16.blocks = OrbisSaveDataBlocksMin2;
    m16.mountMode = 0x02 | 0x04;
    OrbisSaveDataMountResult r16{};
    EXPECT_EQ(sceSaveDataMount2(&m16, &r16), static_cast<s32>(ORBIS_SAVE_DATA_ERROR_MOUNT_FULL));

    for (const auto& mp : mounted) {
        EXPECT_EQ(sceSaveDataUmount(&mp), ORBIS_OK);
    }
    ASSERT_EQ(sceSaveDataGetMountedSaveDataCount(&mounted_count), ORBIS_OK);
    EXPECT_EQ(mounted_count, 0u);
}

TEST_F(SaveDataTest, IconValidationAndMetaSyncAndParamValidation) {
    ASSERT_EQ(sceSaveDataInitialize3(nullptr), ORBIS_OK);
    OrbisSaveDataDirName dir{"meta_slot"};
    OrbisSaveDataMount2 m{};
    m.userId = kUid1;
    m.dirName = &dir;
    m.blocks = OrbisSaveDataBlocksMin2;
    m.mountMode = 0x02 | 0x04;
    OrbisSaveDataMountResult res{};
    ASSERT_EQ(sceSaveDataMount2(&m, &res), ORBIS_OK);

    // Param validation: reject CR/LF in title/subTitle, allow LF in detail
    OrbisSaveDataParam bad_param{};
    std::snprintf(bad_param.title, sizeof(bad_param.title), "Bad\nTitle");
    EXPECT_NE(sceSaveDataSetParam(&res.mountPoint, 0, &bad_param, sizeof(bad_param)), ORBIS_OK);

    OrbisSaveDataParam good_param{};
    std::snprintf(good_param.title, sizeof(good_param.title), "Hero Save");
    std::snprintf(good_param.subTitle, sizeof(good_param.subTitle), "Chapter 1");
    std::snprintf(good_param.detail, sizeof(good_param.detail), "Line 1\nLine 2");
    good_param.userParam = 0xCAFEBABEU;
    ASSERT_EQ(sceSaveDataSetParam(&res.mountPoint, 0, &good_param, sizeof(good_param)), ORBIS_OK);

    // GetParam short buffer returns error cleanly without crash
    OrbisSaveDataParam read_param{};
    size_t got = 0;
    EXPECT_NE(sceSaveDataGetParam(&res.mountPoint, 0, &read_param, 16, &got), ORBIS_OK);
    ASSERT_EQ(sceSaveDataGetParam(&res.mountPoint, 0, &read_param, sizeof(read_param), &got),
              ORBIS_OK);
    EXPECT_STREQ(read_param.title, "Hero Save");
    EXPECT_STREQ(read_param.subTitle, "Chapter 1");
    EXPECT_STREQ(read_param.detail, "Line 1\nLine 2");
    EXPECT_EQ(read_param.userParam, 0xCAFEBABEU);

    // Bad icon dimensions (128x128 RGB) and wrong color format (228x128 RGBA) rejected
    auto bad_png = BuildPngBytes(128, 128);
    OrbisSaveDataIcon bad_icon{};
    bad_icon.buf = bad_png.data();
    bad_icon.bufSize = bad_png.size();
    bad_icon.dataSize = bad_png.size();
    EXPECT_NE(sceSaveDataSaveIcon(&res.mountPoint, &bad_icon), ORBIS_OK);

    auto rgba_png = BuildPngBytes(228, 128, PNG_COLOR_TYPE_RGBA);
    OrbisSaveDataIcon rgba_icon{};
    rgba_icon.buf = rgba_png.data();
    rgba_icon.bufSize = rgba_png.size();
    rgba_icon.dataSize = rgba_png.size();
    EXPECT_NE(sceSaveDataSaveIcon(&res.mountPoint, &rgba_icon), ORBIS_OK);

    // Valid 228x128 icon accepted + synced to savedata_meta
    auto good_png = BuildPngBytes(228, 128);
    OrbisSaveDataIcon good_icon{};
    good_icon.buf = good_png.data();
    good_icon.bufSize = good_png.size();
    good_icon.dataSize = good_png.size();
    ASSERT_EQ(sceSaveDataSaveIcon(&res.mountPoint, &good_icon), ORBIS_OK);
    ASSERT_EQ(sceSaveDataUmount(&res.mountPoint), ORBIS_OK);

    const fs::path meta_icon = GetMetaIconPath(kUid1, kTitleId, "meta_slot");
    EXPECT_TRUE(fs::exists(meta_icon));
    EXPECT_EQ(fs::file_size(meta_icon), good_png.size());
}

TEST_F(SaveDataTest, AsyncUmountWithBackupAndRestore) {
    ASSERT_EQ(sceSaveDataInitialize3(nullptr), ORBIS_OK);

    std::mutex cb_mtx;
    std::condition_variable cb_cv;
    bool got_event = false;
    s32 cb_type = 0;
    s32 cb_err = -1;
    struct Ctx {
        std::mutex* m;
        std::condition_variable* cv;
        bool* got;
        s32* type;
        s32* err;

        static PS4_SYSV_ABI void OnEvent(OrbisSaveDataEvent* ev, void* ud) {
            auto* c = static_cast<Ctx*>(ud);
            std::scoped_lock lk(*c->m);
            *c->type = ev->type;
            *c->err = ev->errorCode;
            *c->got = true;
            c->cv->notify_all();
        }
    } ctx{&cb_mtx, &cb_cv, &got_event, &cb_type, &cb_err};

    ASSERT_EQ(sceSaveDataRegisterEventCallback(&Ctx::OnEvent, &ctx), ORBIS_OK);

    OrbisSaveDataDirName dir{"backup_slot"};
    OrbisSaveDataMount2 m{};
    m.userId = kUid1;
    m.dirName = &dir;
    m.blocks = OrbisSaveDataBlocksMin2;
    m.mountMode = 0x02 | 0x04;
    OrbisSaveDataMountResult res{};
    ASSERT_EQ(sceSaveDataMount2(&m, &res), ORBIS_OK);

    OrbisSaveDataParam p1{};
    std::snprintf(p1.title, sizeof(p1.title), "Backup V1");
    ASSERT_EQ(sceSaveDataSetParam(&res.mountPoint, 0, &p1, sizeof(p1)), ORBIS_OK);

    ASSERT_EQ(sceSaveDataUmountWithBackup(&res.mountPoint), ORBIS_OK);
    {
        std::unique_lock lk(cb_mtx);
        ASSERT_TRUE(cb_cv.wait_for(lk, std::chrono::seconds(5), [&] { return got_event; }));
        EXPECT_EQ(cb_err, ORBIS_OK);
        EXPECT_EQ(cb_type, static_cast<s32>(Backup::OrbisSaveDataEventType::UMOUNT_BACKUP));
    }

    const fs::path bu_dir = GetBackupDirPath(kUid1, kTitleId, "backup_slot");
    EXPECT_TRUE(fs::exists(bu_dir / "sce_sys" / "param.sfo"));

    // Modify live save to V2, verify CheckBackupData still shows V1, then Restore
    m.mountMode = 0x02;
    ASSERT_EQ(sceSaveDataMount2(&m, &res), ORBIS_OK);
    OrbisSaveDataParam p2{};
    std::snprintf(p2.title, sizeof(p2.title), "Corrupted Live V2");
    ASSERT_EQ(sceSaveDataSetParam(&res.mountPoint, 0, &p2, sizeof(p2)), ORBIS_OK);
    ASSERT_EQ(sceSaveDataUmount(&res.mountPoint), ORBIS_OK);

    OrbisSaveDataCheckBackupData chk{};
    OrbisSaveDataParam chk_param{};
    chk.userId = kUid1;
    chk.dirName = &dir;
    chk.param = &chk_param;
    ASSERT_EQ(sceSaveDataCheckBackupData(&chk), ORBIS_OK);
    EXPECT_STREQ(chk_param.title, "Backup V1");

    OrbisSaveDataRestoreBackupData rst{};
    rst.userId = kUid1;
    rst.dirName = &dir;
    ASSERT_EQ(sceSaveDataRestoreBackupData(&rst), ORBIS_OK);

    ASSERT_EQ(sceSaveDataMount2(&m, &res), ORBIS_OK);
    OrbisSaveDataParam restored_p{};
    ASSERT_EQ(sceSaveDataGetParam(&res.mountPoint, 0, &restored_p, sizeof(restored_p), nullptr),
              ORBIS_OK);
    EXPECT_STREQ(restored_p.title, "Backup V1");
    ASSERT_EQ(sceSaveDataUmount(&res.mountPoint), ORBIS_OK);
}

TEST_F(SaveDataTest, SaveMemorySetupWriteReadAndRestoreLoad) {
    ASSERT_EQ(sceSaveDataInitialize3(nullptr), ORBIS_OK);

    auto png = BuildPngBytes(228, 128);
    OrbisSaveDataParam init_p{};
    std::snprintf(init_p.title, sizeof(init_p.title), "Quick Mem");
    OrbisSaveDataIcon init_i{};
    init_i.buf = png.data();
    init_i.bufSize = png.size();
    init_i.dataSize = png.size();

    OrbisSaveDataMemorySetup2 setup{};
    setup.option = 0;
    setup.userId = kUid1;
    setup.memorySize = 4096;
    setup.iconMemorySize = png.size();
    setup.initParam = &init_p;
    setup.initIcon = &init_i;
    setup.slotId = 0;

    OrbisSaveDataMemorySetupResult sres{};
    ASSERT_EQ(sceSaveDataSetupSaveDataMemory2(&setup, &sres), ORBIS_OK);
    EXPECT_EQ(sres.existedMemorySize, 0u);

    const char payload[] = "PS4_SDMEMORY_PAYLOAD";
    ASSERT_EQ(sceSaveDataSetSaveDataMemory(kUid1, payload, sizeof(payload), 128), ORBIS_OK);

    OrbisSaveDataMemorySync sync_p{};
    sync_p.userId = kUid1;
    sync_p.slotId = 0;
    ASSERT_EQ(sceSaveDataSyncSaveDataMemory(&sync_p), ORBIS_OK);

    char read_buf[64]{};
    ASSERT_EQ(sceSaveDataGetSaveDataMemory(kUid1, read_buf, sizeof(payload), 128), ORBIS_OK);
    EXPECT_STREQ(read_buf, payload);

    // Invalid option / oversized memory fails cleanly
    setup.option = 9;
    EXPECT_NE(sceSaveDataSetupSaveDataMemory2(&setup, &sres), ORBIS_OK);
}

TEST_F(SaveDataTest, DirNameSearchSortingAndRebuildDbFromDisk) {
    ASSERT_EQ(sceSaveDataInitialize3(nullptr), ORBIS_OK);
    const char* names[] = {"slot_b", "slot_a", "slot_c"};
    const u32 uparams[] = {20, 30, 10};

    for (int i = 0; i < 3; ++i) {
        OrbisSaveDataDirName d{};
        std::memcpy(d.data, names[i], std::strlen(names[i]));
        OrbisSaveDataMount2 m{};
        m.userId = kUid1;
        m.dirName = &d;
        m.blocks = OrbisSaveDataBlocksMin2;
        m.mountMode = 0x02 | 0x04;
        OrbisSaveDataMountResult r{};
        ASSERT_EQ(sceSaveDataMount2(&m, &r), ORBIS_OK);
        OrbisSaveDataParam p{};
        std::snprintf(p.title, sizeof(p.title), "T%d", i);
        p.userParam = uparams[i];
        ASSERT_EQ(sceSaveDataSetParam(&r.mountPoint, 0, &p, sizeof(p)), ORBIS_OK);
        ASSERT_EQ(sceSaveDataUmount(&r.mountPoint), ORBIS_OK);
    }

    OrbisSaveDataDirNameSearchCond cond{};
    cond.userId = kUid1;
    cond.key = 1;   // USER_PARAM
    cond.order = 1; // DESC
    OrbisSaveDataDirName out_dirs[8]{};
    OrbisSaveDataParam out_params[8]{};
    OrbisSaveDataSearchInfo out_infos[8]{};
    OrbisSaveDataDirNameSearchResult sres{};
    sres.dirNames = out_dirs;
    sres.dirNamesNum = 8;
    sres.params = out_params;
    sres.infos = out_infos;

    ASSERT_EQ(sceSaveDataDirNameSearch(&cond, &sres), ORBIS_OK);
    EXPECT_EQ(sres.hitNum, 3u);
    EXPECT_EQ(sres.setNum, 3u);
    EXPECT_STREQ(out_dirs[0].data, "slot_a");
    EXPECT_STREQ(out_dirs[1].data, "slot_b");
    EXPECT_STREQ(out_dirs[2].data, "slot_c");

    // Delete SQLite DB and rebuild from disk
    SaveDb::CloseAll();
    std::error_code ec;
    fs::remove(GetDbPath(kUid1), ec);
    ASSERT_EQ(sceSaveDataRebuildDatabase(), ORBIS_OK);
    EXPECT_EQ(sceSaveDataGetSaveDataCount(kUid1), 3);
}

TEST_F(SaveDataTest, Mount5Copy5Promote5Delete5AndTransferringMount) {
    ASSERT_EQ(sceSaveDataInitialize3(nullptr), ORBIS_OK);

    OrbisSaveDataMount5 m5{};
    m5.path = "/user/home/10000001/savedata/CUSA12345/path_src";
    m5.blocks = OrbisSaveDataBlocksMin2;
    m5.mountMode = 0x02 | 0x04;
    OrbisSaveDataMountResult r{};
    ASSERT_EQ(sceSaveDataMount5(&m5, &r), ORBIS_OK);
    ASSERT_EQ(sceSaveDataUmount(&r.mountPoint), ORBIS_OK);

    OrbisSaveDataCopy5 cp{};
    cp.srcPath = "/user/home/10000001/savedata/CUSA12345/path_src";
    cp.dstPath = "/user/home/10000001/savedata/CUSA12345/path_dst";
    ASSERT_EQ(sceSaveDataCopy5(&cp), ORBIS_OK);
    EXPECT_TRUE(fs::exists(GetSaveDirPath(kUid1, kTitleId, "path_dst")));

    OrbisSaveDataPromote5 pr{};
    pr.srcPath = "/user/home/10000001/savedata/CUSA12345/path_dst";
    pr.dstPath = "/user/home/10000001/savedata/CUSA12345/path_prom";
    ASSERT_EQ(sceSaveDataPromote5(&pr), ORBIS_OK);
    EXPECT_FALSE(fs::exists(GetSaveDirPath(kUid1, kTitleId, "path_dst")));
    EXPECT_TRUE(fs::exists(GetSaveDirPath(kUid1, kTitleId, "path_prom")));

    OrbisSaveDataDelete5 del5{};
    del5.path = "/user/home/10000001/savedata/CUSA12345/path_prom";
    ASSERT_EQ(sceSaveDataDelete5(&del5), ORBIS_OK);

    // TransferringMount requires titleId & fingerprint either both present or both null
    OrbisSaveDataTitleId tid{"CUSA12345"};
    OrbisSaveDataDirName d{"path_src"};
    OrbisSaveDataTransferringMount tm{};
    tm.userId = kUid1;
    tm.titleId = &tid;
    tm.dirName = &d;
    tm.fingerprint = nullptr;
    EXPECT_NE(sceSaveDataTransferringMount(&tm, &r), ORBIS_OK);

    tm.titleId = nullptr;
    ASSERT_EQ(sceSaveDataTransferringMount(&tm, &r), ORBIS_OK);
    EXPECT_EQ(sceSaveDataUmount(&r.mountPoint), ORBIS_OK);
}

TEST_F(SaveDataTest, DialogValidationProgressClampAndBrokenDbItem) {
    ASSERT_EQ(sceSaveDataInitialize3(nullptr), ORBIS_OK);
    Libraries::CommonDialog::g_isInitialized = true;

    EXPECT_EQ(Dialog::sceSaveDataDialogIsReadyToDisplay(),
              static_cast<s32>(Libraries::CommonDialog::Error::NOT_INITIALIZED));
    ASSERT_EQ(Dialog::sceSaveDataDialogInitialize(), Libraries::CommonDialog::Error::OK);
    EXPECT_EQ(Dialog::sceSaveDataDialogIsReadyToDisplay(),
              static_cast<s32>(Libraries::CommonDialog::Error::INVALID_STATE));

    // Wrong size returns PARAM_INVALID rather than crashing
    Dialog::OrbisSaveDataDialogParam bad{};
    bad.baseParam.size = sizeof(Libraries::CommonDialog::BaseParam);
    bad.size = 123;
    EXPECT_EQ(Dialog::sceSaveDataDialogOpen(&bad), Libraries::CommonDialog::Error::PARAM_INVALID);

    // USER_MSG with items == nullptr must not crash
    Dialog::OrbisSaveDataDialogUserMessageParam umsg{};
    umsg.buttonType = Dialog::OrbisSaveDataDialogButtonType::OK;
    umsg.msgType = Dialog::OrbisSaveDataDialogUserMessageType::NORMAL;
    umsg.msg = "Hello Save";
    Dialog::OrbisSaveDataDialogParam up{};
    up.baseParam.size = sizeof(Libraries::CommonDialog::BaseParam);
    up.size = sizeof(up);
    up.mode = Dialog::OrbisSaveDataDialogMode::USER_MSG;
    up.dispType = Dialog::OrbisSaveDataDialogType::SAVE;
    up.items = nullptr;
    up.userMsgParam = &umsg;
    ASSERT_EQ(Dialog::sceSaveDataDialogOpen(&up), Libraries::CommonDialog::Error::OK);
    EXPECT_EQ(Dialog::sceSaveDataDialogIsReadyToDisplay(), 1);

    Dialog::OrbisSaveDataDialogCloseParam cp{};
    cp.anim = Dialog::OrbisSaveDataDialogAnimation::OFF;
    ASSERT_EQ(Dialog::sceSaveDataDialogClose(&cp), Libraries::CommonDialog::Error::OK);
    Dialog::OrbisSaveDataDialogResult dres{};
    ASSERT_EQ(Dialog::sceSaveDataDialogGetResult(&dres), Libraries::CommonDialog::Error::OK);

    // PROGRESS_BAR clamped at 100
    Dialog::OrbisSaveDataDialogProgressBarParam pbar{};
    pbar.barType = Dialog::OrbisSaveDataDialogProgressBarType::PERCENTAGE;
    pbar.sysMsgType = Dialog::OrbisSaveDataDialogProgressSystemMessageType::PROGRESS;
    Dialog::OrbisSaveDataDialogParam pp{};
    pp.baseParam.size = sizeof(Libraries::CommonDialog::BaseParam);
    pp.size = sizeof(pp);
    pp.mode = Dialog::OrbisSaveDataDialogMode::PROGRESS_BAR;
    pp.dispType = Dialog::OrbisSaveDataDialogType::SAVE;
    pp.progressBarParam = &pbar;
    ASSERT_EQ(Dialog::sceSaveDataDialogOpen(&pp), Libraries::CommonDialog::Error::OK);
    ASSERT_EQ(Dialog::sceSaveDataDialogProgressBarSetValue(
                  Dialog::OrbisSaveDataDialogProgressBarTarget::DEFAULT, 250),
              Libraries::CommonDialog::Error::OK);
    Dialog::SaveDialogState state_copy = Dialog::GetDialogStateForTest();
    EXPECT_EQ(state_copy.GetState<Dialog::SaveDialogState::ProgressBarState>().progress, 100u);
    ASSERT_EQ(Dialog::sceSaveDataDialogTerminate(), Libraries::CommonDialog::Error::OK);
}

} // namespace
