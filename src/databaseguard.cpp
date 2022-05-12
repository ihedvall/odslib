/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include "ods/databaseguard.h"
namespace ods {

DatabaseGuard::DatabaseGuard(IDatabase &database)
 : database_(&database) {
  if (!database.IsOpen()) {
    try {
      database.Open();
      db_ok_ = true;
    } catch (const std::exception&) {
      database_ = nullptr;
    }
  } else {
    database_  = nullptr;
    db_ok_ = true;
  }
}

DatabaseGuard::~DatabaseGuard() {
  if (database_ != nullptr) {
    database_->Close(true);
  }
}

bool DatabaseGuard::IsOk() const {
  return db_ok_;
}

void DatabaseGuard::Rollback() {
  if (database_ != nullptr) {
    database_->Close(false);
    database_ = nullptr;
  }
  db_ok_ = false;
}

}