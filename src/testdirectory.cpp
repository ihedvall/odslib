/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <filesystem>
#include <chrono>
#include <unordered_map>
#include <boost/algorithm/string/case_conv.hpp>
#include <util/logstream.h>
#include <util/stringutil.h>
#include <util/stringparser.h>
#include <util/timestamp.h>
#include <util/cryptoutil.h>
#include "ods/databaseguard.h"
#include "ods/iitem.h"
#include "mdf/mdfreader.h"
#include "testdirectory.h"

using namespace util::log;
using namespace util::string;
using namespace util::time;
using namespace util::crypto;
using namespace mdf;
using namespace std::chrono_literals;
using namespace std::filesystem;

namespace {

template<typename T>
void AddItemColumn(ods::IItem &dest, const ods::ITable &table, bool base, const std::string &name, const T& value) {
  const auto *column = base ? table.GetColumnByBaseName(name) : table.GetColumnByName(name);
  if (column != nullptr && !column->DatabaseName().empty()) {
    dest.AppendAttribute({column->ApplicationName(), column->BaseName(), value});
  }
}

ods::DataType ChannelTypeToDataType(const mdf::IChannel& channel) {
  switch (channel.DataType()) {
    case mdf::ChannelDataType::UnsignedIntegerLe:
    case mdf::ChannelDataType::UnsignedIntegerBe:
      if (channel.DataBytes() <= 1) {
        return ods::DataType::DtShort;
      }
      if (channel.DataBytes() <= 2) {
        return ods::DataType::DtLong;
      }
      return ods::DataType::DtLongLong;

    case mdf::ChannelDataType::SignedIntegerLe:
    case mdf::ChannelDataType::SignedIntegerBe:
      if (channel.DataBytes() <= 1) {
        return ods::DataType::DtByte;
      }
      if (channel.DataBytes() <= 2) {
        return ods::DataType::DtShort;
      }
      if (channel.DataBytes() <= 4) {
        return ods::DataType::DtLong;
      }
      return ods::DataType::DtLongLong;

    case mdf::ChannelDataType::FloatLe:
    case mdf::ChannelDataType::FloatBe:
      if (channel.DataBytes() <= 4) {
        return ods::DataType::DtFloat;
      }
      return ods::DataType::DtDouble;

    case mdf::ChannelDataType::StringAscii:
    case mdf::ChannelDataType::StringUTF8:
    case mdf::ChannelDataType::StringUTF16Le:
    case mdf::ChannelDataType::StringUTF16Be:
    case mdf::ChannelDataType::MimeSample:
    case mdf::ChannelDataType::MimeStream:
      return ods::DataType::DtString;

    case mdf::ChannelDataType::ByteArray:
      return ods::DataType::DtBlob;

    case mdf::ChannelDataType::CanOpenDate:
    case mdf::ChannelDataType::CanOpenTime:
      return ods::DataType::DtDate;

    default:
      break;
  }
  return ods::DataType::DtUnknown;
}

} // end namespace

namespace ods::detail {

TestDirectory::TestDirectory()
: IEnvironment(EnvironmentType::kTypeTestDirectory) {

}

TestDirectory::~TestDirectory() {
  Stop();
}

bool TestDirectory::Init() {
  is_ok_ = false;
  if (db_file_.empty()) {
    LOG_ERROR() << "The database file name has not been set.";
    return false;
  }
  if (root_dir_.empty()) {
    LOG_ERROR() << "The root directory has not been set.";
    return false;
  }

  if (Name().empty()) {
    LOG_ERROR() << "The name has not been set.";
    return false;
  }

  // Check that root dir exist
  bool root_dir_exist = false;
  try {
    std::filesystem::path root(root_dir_);
    root_dir_exist = std::filesystem::exists(root);
  } catch (const std::exception& err) {
    LOG_ERROR() << "Path error in root directory. Error: " << err.what()
                << ", Dir: " << root_dir_;
  }
  if (!root_dir_exist) {
    LOG_ERROR() << "The root directory doesn't exist. Dir: " << root_dir_;
    return false;
  }

  // Check if we need to create the database
  bool need_create_db = false;
  try {
    std::filesystem::path db_path(db_file_);
    need_create_db = !std::filesystem::exists(db_path);
  } catch (const std::exception& err) {
    LOG_ERROR() << "Path error in database path. Error: " << err.what()
                << ", Path: " << db_file_;
    return false;
  }

  if (need_create_db) {
    const auto create = CreateDb();
    if (!create) {
      LOG_ERROR() << "Failed to create the cache database. Path: " << db_file_;
      return false;
    } else {
      LOG_DEBUG() << "Created a database. Database: " << db_file_;
    }
  }

  // No actual initialize the environment
  const auto init = InitDb();
  if (!init) {
    LOG_ERROR() << "Fail to initialize the environment. Environment: " << Name();
    return false;
  } else {
    LOG_DEBUG() << "Read in model from the database. Database: " << DbFileName();
  }

  is_ok_ = true;
  return true;
}

bool TestDirectory::CreateDb() {
  if (ModelFileName().empty()) {
    LOG_ERROR() << "No model file defined. Cannot create a database.";
    return false;
  }
  const auto read = model_.ReadModel(ModelFileName());
  if (!read) {
    LOG_ERROR() << "Failed to read in the model. File: " << ModelFileName();
    return false;
  }
  const auto create = database_.Create(model_);
  if (!create) {
    LOG_ERROR() << "Failed to create the cache database.";
    return false;
  }

  return true;
}

bool TestDirectory::InitDb() {
  if (model_.IsEmpty()) {
    const auto read = database_.ReadModel(model_);
    if (!read) {
      LOG_ERROR() << "Failed to read in the ODS model from the database";
      return false;
    }
  }
  return true;
}

void TestDirectory::DbFileName(const std::string &db_file) {
  db_file_ = db_file;
  database_.FileName(db_file);
}

bool TestDirectory::InsertRow(IItem &row) {
  const auto* table = row.ApplicationId() > 0 ?
      model_.GetTable(row.ApplicationId()) : model_.GetTableByName(row.ApplicationName());
  if (table == nullptr) {
    LOG_ERROR() << "Cannot find the table. Table ID: " << row.ApplicationId();
    return false;
  }
  // Fill in missing values into the row
  auto& item_list = row.AttributeList();
  const auto& column_list = table->Columns();
  for (const auto& column : column_list) {
    if (row.ExistAttribute(column.ApplicationName()) || column.DatabaseName().empty()) {
      continue;
    }
    // Column is missing

    if (!column.DefaultValue().empty()) {
      row.AppendAttribute({column.ApplicationName(), column.DefaultValue()});
    } else if (column.DataType() == DataType::DtEnum) {
      row.AppendAttribute({column.ApplicationName(), 0});
    } else if (IEquals(column.BaseName(), "id")) {
      row.AppendAttribute({column.ApplicationName(), 0LL});
    } else if (IEquals(column.BaseName(), "factor") && column.Obligatory()) {
      row.AppendAttribute({column.ApplicationName(), 1.0});
    } else if (IEquals(column.BaseName(), "offset") && column.Obligatory()) {
      row.AppendAttribute({column.ApplicationName(), 0.0});
    } else if (column.ReferenceId() > 0 && column.Obligatory() ) {
      row.AppendAttribute({column.ApplicationName(), 0LL});
    }
  }

  try {
    database_.Insert(*table, row);
  } catch (const std::exception& err) {
    LOG_ERROR() << "InsertRow Failed. Error: " << err.what();
    return false;
  }
  return true;
}

std::string TestDirectory::ExcludeListToText() const {
  std::ostringstream temp;
  size_t count = 0;
  for (const auto& text : exclude_list_) {
    if (text.empty()) {
      continue;
    }
    if ( count > 0) {
      temp << ";";
    }
    temp << text;
    ++count;
  }
  return temp.str();
}

void TestDirectory::TextToExcludeList(const std::string& text) {
  exclude_list_.clear();
  std::ostringstream temp;
  for (const char cin : text) {
    switch (cin) {
      case ';':
      case ':':
      case ',':
      case '|': {
        auto text1 = Trim(temp.str());
        if (!text1.empty()) {
          exclude_list_.emplace_back(text1);
        }
        temp.str("");
        temp.clear();
        break;
      }

      default:
        temp << cin;
        break;
    }
  }
  auto text2 = Trim(temp.str());
  if (!text2.empty()) {
    exclude_list_.emplace_back(text2);
  }
}

bool TestDirectory::FetchFromDb() {
  test_list_.clear();
  test_bed_list_.clear();
  quantity_list_.clear();
  unit_list_.clear();

  const auto* test_bed_table = model_.GetTableByName("TestBed");
  const auto* test_table = model_.GetBaseId(BaseId::AoTest);
  const auto* quantity_table = model_.GetBaseId(BaseId::AoQuantity);
  const auto* unit_table = model_.GetBaseId(BaseId::AoUnit);

  DatabaseGuard db_lock(database_);
  try {
    if (test_bed_table != nullptr) {
      database_.FetchItemList(*test_bed_table, test_bed_list_);
    }
    if (test_table != nullptr) {
      database_.FetchItemList(*test_table, test_list_);
    }
    if (quantity_table != nullptr) {
      database_.FetchItemList(*quantity_table, quantity_list_);
    }
    if (unit_table != nullptr) {
      database_.FetchItemList(*unit_table, unit_list_);
    }
  } catch (const std::exception& err) {
    LOG_ERROR() << "Fetching from DB failed. Error: " << err.what();
    return false;
  }
  return true;
}

bool TestDirectory::IsOk() const {
  return is_ok_;
}

void TestDirectory::Start() {
  if (IsStarted()) {
    LOG_DEBUG() << "Start called on started worker thread.";
    return;
  }
  if (!IsOk()) {
    LOG_ERROR() << "Init Failed does not start the worker thread. ";
    return;
  }

  stop_thread_ = false;
  worker_thread_ = std::thread(&TestDirectory::WorkerThread, this);
}

bool TestDirectory::IsStarted() const {
  return worker_thread_.joinable() && stop_thread_ == false;
}

void TestDirectory::Stop() {
  stop_thread_ = true;
  worker_condition_.notify_one();
  LOG_DEBUG() << "Worker thread request to stop. Environment: " << Name();
  if (worker_thread_.joinable()) {
    worker_thread_.join();
  }
  LOG_DEBUG() << "Worker thread stopped. Environment: " << Name();
}

void TestDirectory::WorkerThread() {
  is_ok_ = true;
  LOG_DEBUG() << "Worker thread started. Environment: " << Name();

  while (!stop_thread_) {
    bool is_ok = true;
    test_dir_list_.clear();
    update_list_.clear();

    // Read in existing test directories from the database.
    const auto fetch_db = FetchFromDb();

    // Scan in new test directories
    const auto scan_root_dir = ScanRootDir();

    // Add any test beds as the index might be needed afterwards
    const auto handle_test_bed = AddNewTestBed();

    // Add and update tests
    const auto handle_test = AddTest();

    // Delete tests
    const auto delete_test = DeleteTest();
    is_ok = fetch_db && scan_root_dir && handle_test && delete_test;

    test_dir_list_.clear(); // Done with this list.

    if (!update_list_.empty() && is_ok) {
      LOG_DEBUG() << "Scan Update Test";
      const auto scan_update_test = ScanUpdateTest();
      LOG_DEBUG() << "Update Test File";
      const auto update_test_file = UpdateTestFile();
      LOG_DEBUG() << "Update Meas File";
      const auto update_meas_file = UpdateMeasFile();
      LOG_DEBUG() << "UpdateReady";
      is_ok = scan_update_test && update_test_file && update_meas_file;
    }
    if (is_ok != is_ok_) {
      is_ok_ = is_ok;
      // Generate an event and log message
    }
    update_list_.clear();

    test_bed_list_.clear();
    test_list_.clear();
    quantity_list_.clear();
    unit_list_.clear();

    std::unique_lock lock(worker_lock_);
    worker_condition_.wait_for(lock, 60s,  [&] {
        return stop_thread_.load();
    });

  }
  LOG_DEBUG() << "Worker thread ready. Environment: " << Name();
  is_ok_ = true;
}

bool TestDirectory::ScanRootDir() {
  StringParser dir_parser(test_dir_format_);

  try {
    path root_dir(RootDir());

    if (!exists(root_dir)) {
      throw std::runtime_error("Root directory doesn't exist.");
    }
    test_dir_list_.clear();
    for ( const auto& entry : directory_iterator(root_dir)) {
      if (!entry.is_directory()) {
        continue;
      }

      TestDir test_dir;
      test_dir.name = entry.path().stem().string();
      const auto parse = dir_parser.Parse(test_dir.name);
      if (!parse) {
        continue;
      }
      test_dir.test_bed = dir_parser.GetTagValue("TestBed");
      if (dir_parser.ExistTag("IsoTime")) {
        test_dir.created = IsoTimeToNs(dir_parser.GetTagValue("IsoTime"), false);
      } else if (dir_parser.ExistTag("LocalIsoTime")) {
        test_dir.created = IsoTimeToNs(dir_parser.GetTagValue("LocalIsoTime"), true);
      }

      test_dir.modified = FileTimeToNs(entry.last_write_time());
      test_dir.modified /= 1'000'000'000; // Normalize to the closest second
      test_dir.modified *= 1'000'000'000;

      test_dir_list_.emplace_back(test_dir);
    }
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to scan the root directory. Error: " << err.what()
        <<", Root Dir: " << RootDir();
    return false;
  }

  return true;
}

/**
 * Scan through the test directories and update the test bed index which
 * is needed when adding and updating tests.
 * @return True on success
 */
bool TestDirectory::AddNewTestBed() {
  const auto* table = model_.GetTableByName("TestBed");
  std::map<std::string, int64_t, IgnoreCase> index_list;
  for (auto& test_dir : test_dir_list_) {
    if (test_dir.test_bed.empty()) {
      continue;
    }
      // Already handled
    const auto find = index_list.find(test_dir.test_bed);
    if (find != index_list.cend()) {
      test_dir.test_bed_index = find->second;
      continue;
    }

      // Check In DB
    const auto itr = std::ranges::find_if(test_bed_list_, [&] (const auto& item) {
      return item && IEquals(item->Name(), test_dir.test_bed);
    });
    if (itr != test_bed_list_.cend()) {
       test_dir.test_bed_index = (*itr)->ItemId();
      index_list.insert({(*itr)->Name(), (*itr)->ItemId()});
      continue;
    }
    if (table != nullptr) {
      IItem item;
      item.ApplicationId(table->ApplicationId());
      const auto* name_column = table->GetColumnByBaseName("name");
      if (name_column != nullptr && !name_column->DatabaseName().empty()) {
        item.AppendAttribute({name_column->ApplicationName(), name_column->BaseName(), test_dir.test_bed});
      }
      // We open and close the database for each test bed as they are rarely added.
      DatabaseGuard db_lock(database_);
      try {
        database_.Insert(*table,item);
      } catch (const std::exception& err) {
        LOG_ERROR() << "Failed to insert test bed. Test Bed: " << test_dir.test_bed
          << ", Error: " << err.what();
        db_lock.Rollback();
        return false;
      }
      test_dir.test_bed_index = item.ItemId();
      index_list.insert({item.Name(), item.ItemId()});
    }
  }

  return true;
}

bool TestDirectory::AddTest() {
  // Scan through first and if needed, open the database and do the update.
  update_list_.clear();
  std::vector<TestDir*> add_list;
  for (auto& test_dir : test_dir_list_) {
    if (test_dir.name.empty() || test_dir.index > 0) {
      continue;
    }
    // Check In DB
    const auto itr = std::ranges::find_if(test_list_, [&] (const auto &item) {
      return item && IEquals(item->Name(), test_dir.name);
    });
    // If existing
    if (itr != test_list_.cend()) {
      test_dir.index = (*itr)->ItemId(); // Mark it as updated
      if (test_dir.modified > (*itr)->Modified()) {
        update_list_.emplace_back(test_dir);
      }
      continue;
    }
    add_list.push_back(&test_dir);
  }

  if (add_list.empty() && update_list_.empty()) {
    return true;
  }

  const auto* table = model_.GetBaseId(BaseId::AoTest);
  if (table == nullptr || table->DatabaseName().empty()) {
    return true;
  }

  DatabaseGuard db_lock(database_);
  for (auto* add : add_list) {
    IItem item;
    item.ApplicationId(table->ApplicationId());

    const auto* name_column = table->GetColumnByBaseName("name");
    if (name_column != nullptr && !name_column->DatabaseName().empty()) {
      item.AppendAttribute({name_column->ApplicationName(), name_column->BaseName(), add->name});
    }

    const auto* created_column = table->GetColumnByBaseName("ao_created");
    if (created_column != nullptr && !created_column->DatabaseName().empty()) {
      item.AppendAttribute({created_column->ApplicationName(), created_column->BaseName(),
                         NsToIsoTime(add->created)});
    }

    const auto* test_bed_column = table->GetColumnByName("TestBed");
    if (test_bed_column != nullptr && !test_bed_column->DatabaseName().empty()) {
      item.AppendAttribute({test_bed_column->ApplicationName(), test_bed_column->BaseName(),
                         add->test_bed_index});
    }

    const auto* modified_column = table->GetColumnByBaseName("ao_last_modified");
    if (modified_column != nullptr && !modified_column->DatabaseName().empty()) {
      item.AppendAttribute({modified_column->ApplicationName(), modified_column->BaseName(),
                         NsToIsoTime(add->modified)});
    }

    try {
      database_.Insert(*table,item);
    } catch (const std::exception& err) {
      LOG_ERROR() << "Failed to insert test. Test: " << add->name
                  << ", Error: " << err.what();
      db_lock.Rollback();
      return false;
    }
    add->index = item.ItemId();
    update_list_.push_back(*add); // Fix a scan of files and a DB update
  }

  return true;
}

bool TestDirectory::DeleteTest() {

  std::vector<int64_t> del_list;
  for (const auto& item : test_list_) {
    if (!item) {
      continue;
    }
    const auto exist = std::ranges::any_of(test_dir_list_, [&] (const auto& test_dir){
      return !test_dir.name.empty() && IEquals(item->Name(), test_dir.name);
    });
    if (exist) {
      continue;
    }
    del_list.push_back(item->ItemId());
  }
  if (del_list.empty()) {
    return true;
  }

  const auto* table = model_.GetBaseId(BaseId::AoTest);
  if (table == nullptr || table->DatabaseName().empty()) {
    return true;
  }

  const auto* index_column = table->GetColumnByBaseName("id");
  if (index_column == nullptr || index_column->DatabaseName().empty()) {
    return true;
  }

  DatabaseGuard db_lock(database_);
  for (int64_t index : del_list) {
    std::ostringstream sql;
    sql << "DELETE FROM " << table->DatabaseName()
        << " WHERE " << index_column->DatabaseName() << " = " << index;

    try {
      database_.ExecuteSql(sql.str());
    } catch (const std::exception &err) {
      LOG_ERROR() << "Failed to delete test. Error: " << err.what();
      db_lock.Rollback();
      return false;
    }
  }
  return true;
}

bool TestDirectory::ScanUpdateTest() {
  if (update_list_.empty()) {
    return true;
  }
  for (auto& test_dir : update_list_) {
    if (test_dir.name.empty()) {
      continue;
    }
    try {
      path dir(RootDir());
      dir.append(test_dir.name);

      if (!exists(dir)) {
        continue;
      }
      auto dir_itr = recursive_directory_iterator(dir);
      for ( const auto& entry :dir_itr) {
        if (entry.is_directory()) {
          const auto dir_name = entry.path().stem().string();
          const auto skip = std::ranges::any_of(exclude_list_, [&] (const auto& filter) {
            return WildcardMatch(dir_name, filter, true);
          });
          if (skip ) {
            dir_itr.disable_recursion_pending(); // Skip this dir
          }
        } else {
          TestFile file;
          const auto& full_name = entry.path();
          file.full_name = full_name.string();
          file.name = full_name.filename().string();
          const auto ext = full_name.extension().string();
          const auto skip = std::ranges::any_of(exclude_list_, [&] (const auto& filter) {
            return WildcardMatch(file.name, filter, true) || WildcardMatch(ext, filter, true);
          });
          if (skip) {
            continue;
          }
          file.size = entry.file_size();
          file.type = boost::to_upper_copy(ext.size() > 1 ? ext.substr(1) : "");
          file.modified = FileTimeToNs(entry.last_write_time());
          file.modified /= 1'000'000'000; // Normalize to the closest second
          file.modified *= 1'000'000'000;

          test_dir.file_list.emplace_back(file);
        }
      }
    } catch (const std::exception& err) {
      LOG_ERROR() << "Failed to scan the test directory. Error: " << err.what()
                  <<", Test Dir: " << test_dir.name;
      return false;
    }

  }
  return true;
}

bool TestDirectory::UpdateTestFile() {
  const auto* table = model_.GetTableByName("TestFile");
  if (table == nullptr || table->DatabaseName().empty()) {
    return true;
  }

  const auto* parent_column = table->GetColumnByBaseName("ao_file_parent");
  if (parent_column == nullptr || parent_column->DatabaseName().empty()) {
    return true;
  }

  const auto* id_column = table->GetColumnByBaseName("id");
  if (id_column == nullptr || id_column->DatabaseName().empty()) {
    return true;
  }

  // Fetch existing files from the database
  DatabaseGuard db_lock(database_);
  for (auto& test_dir : update_list_) {
    SqlFilter pix;
    pix.AddWhere(*parent_column,SqlCondition::Equal,test_dir.index);
    ItemList db_list;
    try {
      database_.FetchItemList(*table, db_list, pix);
    } catch (const std::exception& err) {
      LOG_ERROR() << "Failed to fetch test files. Test: " << test_dir.name
                  << ", Error: " << err.what();
      db_lock.Rollback();
      return false;
    }

    for (auto& test_file : test_dir.file_list) {
      if (test_file.name.empty()) {
        continue;
      }
      const auto exist = std::ranges::find_if(db_list, [&] (const auto& ptr) {
        return ptr && IEquals(ptr->Name(), test_file.name);
      });

      IItem item;
      item.ApplicationId(table->ApplicationId());
      AddItemColumn(item, *table, true,"ao_location", test_file.full_name);
      AddItemColumn(item, *table, true,"ao_file_mimetype", test_file.type);
      AddItemColumn(item, *table, true,"ao_size", test_file.size);
      AddItemColumn(item, *table, true,"ao_last_modified", NsToIsoTime(test_file.modified));
      // AddItemColumn(item, *table, true,"ao_hash_algorithm", "MD5");
      if (exist != db_list.cend()) {
        // Update
        test_file.index = (*exist)->ItemId();
        if (test_file.modified > (*exist)->Modified()) {
          // Update file
          test_file.is_modified = true;
          //AddItemColumn(item, *table, true,"ao_hash_value", CreateMd5FileString(test_file.full_name));
          try {
            database_.Update(*table,item, pix);
          } catch (const std::exception& err) {
            LOG_ERROR() << "Failed to update test file. Test: " << test_dir.name << ", File: " << test_file.name
                        << ", Error: " << err.what();
            db_lock.Rollback();
            return false;
          }
        }
      } else {
        // Insert file
        test_file.is_modified = true;
        AddItemColumn(item, *table, true,"name", test_file.name);
        AddItemColumn(item, *table, true,"ao_file_parent", test_dir.index);
        //AddItemColumn(item, *table, true,"ao_hash_value", CreateMd5FileString(test_file.full_name));
        try {
          database_.Insert(*table,item);
          test_file.index = item.ItemId();
        } catch (const std::exception& err) {
          LOG_ERROR() << "Failed to insert test file. Test: " << test_dir.name << ", File: " << test_file.name
                      << ", Error: " << err.what();
          db_lock.Rollback();
          return false;
        }
      }
    }

      // Check if any files needs to be deleted
    for ( const auto& del : db_list) {

      if (!del || del->Name().empty()) {
        continue;
      }
      const auto exist = std::ranges::any_of(test_dir.file_list, [&] (const auto& file) {
        return IEquals(del->Name(),file.name);
      });
      if (exist) {
        continue;
      }

      SqlFilter file_ix;
      file_ix.AddWhere(*id_column,SqlCondition::Equal,del->ItemId());
      try {
        database_.Delete(*table,file_ix);
      } catch (const std::exception& err) {
        LOG_ERROR() << "Failed to delete test file. Test: " << test_dir.name << ", File: " << del->Name()
                    << ", Error: " << err.what();
        db_lock.Rollback();
        return false;
      }
    }
  }

  return true;
}

bool TestDirectory::UpdateMeasFile() {
  const auto* table = model_.GetTableByName("MeasFile");
  if (table == nullptr || table->DatabaseName().empty()) {
    return true;
  }

  const auto* parent_column = table->GetColumnByBaseName("parent_test");
  if (parent_column == nullptr || parent_column->DatabaseName().empty()) {
    return true;
  }

  const auto* id_column = table->GetColumnByBaseName("id");
  if (id_column == nullptr || id_column->DatabaseName().empty()) {
    return true;
  }

  // Fetch existing files from the database
  DatabaseGuard db_lock(database_);
  for (auto& test_dir : update_list_) {
    SqlFilter pix;
    pix.AddWhere(*parent_column,SqlCondition::Equal,test_dir.index);
    ItemList db_list;
    try {
      database_.FetchItemList(*table, db_list, pix);
    } catch (const std::exception& err) {
      LOG_ERROR() << "Failed to fetch measurement files. Test: " << test_dir.name
                  << ", Error: " << err.what();
      db_lock.Rollback();
      return false;
    }

    for (auto& test_file : test_dir.file_list) {
      if (test_file.name.empty()|| test_file.index <= 0 || test_dir.index <= 0) {
        continue;
      }
      const auto is_mdf = IsMdfFile(test_file.full_name);
      if (!is_mdf) {
        continue;
      }
        // Read in all data from the MDF file
      MdfReader reader(test_file.full_name);
      const auto is_ok = reader.IsOk();
      const auto read = reader.ReadEverythingButData();
      const auto* meas_file = reader.GetFile();
      const auto* header = meas_file != nullptr ? meas_file->Header() : nullptr;
      if (!is_ok || !read || meas_file == nullptr || header == nullptr) {
        LOG_ERROR() << "Failure to read MDF file. Test: " << test_dir.name
                    << ", File: " << test_file.name;
        continue;
      }

        // Insert or update
      const auto exist = std::ranges::find_if(db_list, [&] (const auto& ptr) {
        return ptr && IEquals(ptr->Name(), test_file.name);
      });

      if (exist != db_list.cend()) {
        test_file.meas_index = (*exist)->ItemId();
        // Meas File doesn't need to be updated but the Meas table needs to be updated

      } else {
          // Insert Meas file and update Meas
        IItem item;
        item.ApplicationId(table->ApplicationId());
        AddItemColumn(item, *table, true,"name", test_file.name);
        AddItemColumn(item, *table, true,"parent_test", test_dir.index);
        AddItemColumn(item, *table, false,"TestFile", test_file.index);
        AddItemColumn(item, *table, true,"version_date", NsToIsoTime(header->StartTime()));
        AddItemColumn(item, *table, true,"version", meas_file->Version());
        AddItemColumn(item, *table, false,"ProgramId", meas_file->ProgramId());
        AddItemColumn(item, *table, true,"description", header->Description());
        AddItemColumn(item, *table, false,"Author", header->Author());
        AddItemColumn(item, *table, false,"Department", header->Department());
        AddItemColumn(item, *table, false,"Project", header->Project());
        AddItemColumn(item, *table, false,"MeasurementId", header->MeasurementId());
        AddItemColumn(item, *table, false,"RecorderId", header->RecorderId());
        AddItemColumn(item, *table, false,"RecorderIndex", header->RecorderIndex());

        try {
          database_.Insert(*table,item);
          test_file.meas_index = item.ItemId();
        } catch (const std::exception& err) {
          LOG_ERROR() << "Failed to insert measurement file. Test: " << test_dir.name << ", File: " << test_file.name
                      << ", Error: " << err.what();
          db_lock.Rollback();
          return false;
        }
      }
      const auto update_meas = UpdateMeas(*meas_file, test_file.meas_index);
      if (!update_meas) {
        LOG_ERROR() << "Failed to update measurements. Test: " << test_dir.name << ", File: " << test_file.name;
        db_lock.Rollback();
        return false;
      }
    }

    // Check if any files needs to be deleted
    for ( const auto& del : db_list) {

      if (!del || del->Name().empty()) {
        continue;
      }
      const auto exist = std::ranges::any_of(test_dir.file_list, [&] (const auto& file) {
        return IEquals(del->Name(),file.name);
      });
      if (exist) {
        continue;
      }

      SqlFilter file_ix;
      file_ix.AddWhere(*id_column,SqlCondition::Equal,del->ItemId());
      try {
        database_.Delete(*table,file_ix);
      } catch (const std::exception& err) {
        LOG_ERROR() << "Failed to delete measurement file. Test: " << test_dir.name << ", File: " << del->Name()
                    << ", Error: " << err.what();
        db_lock.Rollback();
        return false;
      }
    }
  }

  return true;
}

bool TestDirectory::UpdateMeas(const MdfFile &meas_file, int64_t parent_index) {
  const auto* table = model_.GetBaseId(BaseId::AoMeasurement);
  if (table == nullptr || table->DatabaseName().empty() || parent_index <= 0) {
    return true;
  }
  const auto* parent_column = table->GetColumnByBaseName("test");
  if (parent_column == nullptr || parent_column->DatabaseName().empty()) {
    return true;
  }

  SqlFilter pix;
  pix.AddWhere(*parent_column,SqlCondition::Equal,parent_index);
  ItemList db_list;
  try {
    database_.FetchItemList(*table, db_list, pix);
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to fetch measurements from the database. Name: " << meas_file.Name()
                << ", Error: " << err.what();
    return false;
  }

  // Scan through all DG blocks
  DataGroupList dg_list;
  meas_file.DataGroups(dg_list);
  long dg_index = 0;
  for (const auto* data_group : dg_list) {
    if (data_group == nullptr) {
      continue;
    }
    // Insert or update
    const auto exist = std::ranges::find_if(db_list, [&] (const std::unique_ptr<IItem>& ptr) {
      return ptr && dg_index == ptr->Value<long>("MeasIndex");
    });

    int64_t meas_index = 0;
    std::string name;
    if (exist != db_list.cend()) {
      // Meas and MQ doesn't need to be updated but the sub-matrix might need to be updated
      meas_index = (*exist)->ItemId();
      name = (*exist)->Name();
    } else {
      // Insert Meas
      name = data_group->Description();
      if (name.empty()) {
        std::ostringstream temp;
        temp << "Measurement " << dg_index + 1;
        name = temp.str();
      }

      IItem item;
      item.ApplicationId(table->ApplicationId());
      AddItemColumn(item, *table, true,"name", name);
      AddItemColumn(item, *table, true,"test", parent_index);
      AddItemColumn(item, *table, true,"ao_storage_type", static_cast<int>(StorageType::kForeignFormat));
      AddItemColumn(item, *table, false,"MeasIndex", dg_index);
      try {
        database_.Insert(*table,item);
        meas_index = item.ItemId();
      } catch (const std::exception& err) {
        LOG_ERROR() << "Failed to insert measurement. File: " << meas_file.Name()
                    << "Meas: " << name << ", Error: " << err.what();
        return false;
      }
    }

    const auto update_mq = UpdateMq(*data_group, meas_index);
    if (!update_mq) {
      LOG_ERROR() << "Failed to update measurement quantities. File: " << meas_file.Name()
                  << ", Meas: " << name;
      return false;
    }
    ++dg_index;
  }

  // We don't Check if any measurement needs to be deleted as deleting measurement rarely happens.

  return true;
}

bool TestDirectory::UpdateMq(const IDataGroup &data_group, int64_t parent_index) {
  const auto* table = model_.GetBaseId(BaseId::AoMeasurementQuantity);
  if (table == nullptr || table->DatabaseName().empty() || parent_index <= 0) {
    return true;
  }
  const auto* parent_column = table->GetColumnByBaseName("measurement");
  if (parent_column == nullptr || parent_column->DatabaseName().empty()) {
    return true;
  }

  const auto* id_column = table->GetColumnByBaseName("id");
  if (id_column == nullptr || id_column->DatabaseName().empty()) {
    return true;
  }

  SqlFilter pix;
  pix.AddWhere(*parent_column,SqlCondition::Equal,parent_index);
  ItemList db_list;
  try {
    database_.FetchItemList(*table, db_list, pix);
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to fetch measurement quantities from the database. Name: " << data_group.Description()
                << ", Error: " << err.what();
    return false;
  }

  // Scan through all DG blocks
  const auto cg_list = data_group.ChannelGroups();
  for (const auto* channel_group : cg_list) {
    if (channel_group == nullptr) {
      continue;
    }

    const auto nof_samples = channel_group->NofSamples();
    const auto channel_list = channel_group->Channels();
    for (const auto* channel : channel_list) {
      if (channel == nullptr || channel->Name().empty()) {
        continue;
      }
      int64_t mq_index = 0;
      const std::string name = channel->Name();
      // Insert or update


      const auto exist = std::ranges::find_if(db_list, [&] (const std::unique_ptr<IItem>& ptr) {
        return ptr && IEquals( name,ptr->Name());
      });

      if (exist != db_list.cend()) {
        // MQ only needs to be updated if nof samples are bigger
        const std::unique_ptr<IItem>& mq_item = *exist;

        mq_index = mq_item->ItemId();
        const auto samples = mq_item->Value<uint64_t>("Samples");

        if (nof_samples > samples) {
          // Update
          SqlFilter parent;
          parent.AddWhere(*id_column,SqlCondition::Equal, mq_index);
          IItem item;
          item.ApplicationId(table->ApplicationId());
          AddItemColumn(item, *table, false,"Samples", nof_samples);
          try {
            database_.Update(*table,item, parent);
          } catch (const std::exception& err) {
            LOG_ERROR() << "Failed to update measurement quantity. Name: " << name;
            return false;
          }
        }
      } else {
        // Insert MQ
        int64_t quantity_index = UpdateQuantity(*channel);
        int64_t unit_index = UpdateUnit(channel->Unit());
        const auto independent = channel->Type() == ChannelType::Master || channel->Type() == ChannelType::VirtualMaster;
        auto item = std::make_unique<IItem>();
        item->ApplicationId(table->ApplicationId());
        AddItemColumn(*item, *table, true,"name", name);
        AddItemColumn(*item, *table, true,"measurement", parent_index);
        AddItemColumn(*item, *table, true,"datatype", static_cast<long>(ChannelTypeToDataType(*channel)));
        AddItemColumn(*item, *table, true,"quantity", quantity_index);
        AddItemColumn(*item, *table, true,"unit", unit_index);
        AddItemColumn(*item, *table, false,"Samples", nof_samples);
        AddItemColumn(*item, *table, false,"Independent", independent);
        try {
          database_.Insert(*table,*item);
          mq_index = item->ItemId();
        } catch (const std::exception& err) {
          LOG_ERROR() << "Failed to insert measurement quantity. Name: " << name
                      << ", Error: " << err.what();
          return false;
        }
        db_list.push_back(std::move(item)); // In case MQ is in multiple channel groups
      }
    }
  }

  return true;
}

int64_t TestDirectory::UpdateQuantity(const IChannel& channel) {
  const auto name = channel.Name();
  if (name.empty()) {
    return 0;
  }

  const auto itr = std::ranges::find_if(quantity_list_, [&] (const std::unique_ptr<IItem>& ptr) {
    const auto mq_name = ptr->Value<std::string>("MqName");
    return ptr && !mq_name.empty() && IEquals(mq_name, name);
  });

  if (itr != quantity_list_.cend()) {
    return (*itr)->ItemId();
  }

  const auto* table = model_.GetBaseId(BaseId::AoQuantity);
  if (table == nullptr || table->DatabaseName().empty()) {
    return 0;
  }

  int64_t unit_index = UpdateUnit(channel.Unit());
  // Insert Quantity
  auto item = std::make_unique<IItem>();
  item->ApplicationId(table->ApplicationId());
  AddItemColumn(*item, *table, true,"name", name);
  AddItemColumn(*item, *table, true,"description", channel.Description());
  AddItemColumn(*item, *table, true,"default_datatype", static_cast<long>(ChannelTypeToDataType(channel)));
  AddItemColumn(*item, *table, true,"default_mq_name", name);
  AddItemColumn(*item, *table, true,"default_unit", unit_index);
  try {
    database_.Insert(*table,*item);
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to insert quantity. Quantity: " << name
                << ", Error: " << err.what();
    return false;
  }
  const int64_t index = item->ItemId();
  quantity_list_.push_back(std::move(item));
  return index;
}

int64_t TestDirectory::UpdateUnit(const std::string &unit) {
  if (unit.empty()) {
    return 0;
  }

  const auto itr = std::ranges::find_if(unit_list_, [&] (const std::unique_ptr<IItem>& ptr) {
    return ptr && ptr->Name() == unit;
  });
  if (itr != unit_list_.cend()) {
    return (*itr)->ItemId();
  }
  const auto* table = model_.GetBaseId(BaseId::AoUnit);
  if (table == nullptr || table->DatabaseName().empty()) {
    return 0;
  }

  // Insert Unit
  auto item = std::make_unique<IItem>();
  item->ApplicationId(table->ApplicationId());
  AddItemColumn(*item, *table, true,"name", unit);
  AddItemColumn(*item, *table, true,"factor", 1.0);
  AddItemColumn(*item, *table, true,"offset", 0.0);
  AddItemColumn(*item, *table, true,"phys_dimension",0);
  try {
    database_.Insert(*table,*item);
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to insert unit. Unit: " << unit
                << ", Error: " << err.what();
    return false;
  }
  const int64_t index = item->ItemId();
  unit_list_.push_back(std::move(item));
  return index;
}

bool TestDirectory::FetchNameIdMap(const ITable &table, NameIdMap &dest_list) {
  DatabaseGuard db_lock(database_);
  try {
    IdNameMap temp_list;
    database_.FetchNameMap(table,temp_list);
    for (const auto& itr : temp_list) {
      dest_list.insert({itr.second, itr.first});
    }
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to get name-id map. Table: " << table.ApplicationName()
                << ", Error: " << err.what();
    db_lock.Rollback();
    return false;
  }
  return true;
}

} // end namespace