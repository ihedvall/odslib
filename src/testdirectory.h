/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <string>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <mdf/mdffile.h>
#include "ods/ienvironment.h"
#include "ods/iitem.h"

#include "sqlitedatabase.h"


namespace ods::detail {


class TestDirectory : public IEnvironment {
 public:
  TestDirectory();
  ~TestDirectory() override;

  [[nodiscard]] const std::string& RootDir() const {
    return root_dir_;
  }
  void RootDir(const std::string& root_dir) {
    root_dir_ = root_dir;
  }

  [[nodiscard]] const std::string& DbFileName() const {
    return db_file_;
  }
  void DbFileName(const std::string& db_file);



  [[nodiscard]] const std::string& TestDirFormat() const {
    return test_dir_format_;
  }
  void TestDirFormat(const std::string& format) {
    test_dir_format_ = format;
  }

  [[nodiscard]] const std::vector<std::string>& ExcludeList() const {
    return exclude_list_;
  }
  [[nodiscard]] std::vector<std::string>& ExcludeList() {
    return exclude_list_;
  }

  [[nodiscard]] std::string ExcludeListToText() const;
  void TextToExcludeList(const std::string& text);

  [[nodiscard]] bool IsOk() const override;
  [[nodiscard]] bool Init() override;
  [[nodiscard]] bool IsStarted() const override;
  void Start() override;
  void Stop() override;

  bool FetchNameIdMap(const ITable& table, NameIdMap& dest_list) override;

 protected:
  bool InsertRow(IItem& row);


 private:

  struct TestFile {
    int64_t index = 0;
    int64_t meas_index = 0;
    bool is_modified = false;
    std::string name;
    std::string full_name;
    std::string type;
    uint64_t    size = 0;
    uint64_t    modified = 0;
  };

  struct TestDir {
    int64_t     index = 0;
    std::string name;
    int64_t     test_bed_index = 0;
    std::string test_bed;
    uint64_t    created = 0;
    uint64_t    modified = 0;
    std::vector<TestFile> file_list;
  };

  using TestDirList = std::vector<TestDir>;

  std::string root_dir_; ///< Directory where to find all the tests with its measurement files.
  std::string db_file_;  ///< Database file name with full path.
  std::string test_dir_format_ = "<TestBed>_<IsoTime>_<Order>";
  std::vector<std::string> exclude_list_;

  SqliteDatabase database_;

  std::atomic<bool> is_ok_ = false;
  std::atomic<bool> stop_thread_ = false;
  std::thread worker_thread_;
  std::mutex  worker_lock_;
  std::condition_variable worker_condition_;
  ItemList test_bed_list_; ///< Temporary list of test beds in the database
  ItemList test_list_;     ///< Temporary list of tests in the database
  ItemList quantity_list_;    ///< Temporary list of quantities in the database
  ItemList unit_list_;    ///< Temporary list of units in the database

  TestDirList test_dir_list_; ///< Temporary list of test directories in root directory
  TestDirList update_list_;   ///< Temporary list of directories that needs an update

  [[nodiscard]] bool CreateDb();
  [[nodiscard]] bool InitDb();
  bool FetchFromDb();
  void WorkerThread();
  bool ScanRootDir();
  bool AddNewTestBed();
  bool AddTest();
  bool DeleteTest();
  bool ScanUpdateTest();
  bool UpdateTestFile();
  bool UpdateMeasFile();
  bool UpdateMeas(const mdf::MdfFile& meas_file, int64_t parent_index);
  bool UpdateMq(const mdf::IDataGroup& data_group, int64_t parent_index);
  int64_t UpdateQuantity(const mdf::IChannel& channel);
  int64_t UpdateUnit(const std::string& unit);
};

} // end namespace



