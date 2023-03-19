/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <wx/wx.h>

namespace ods::gui {

class PostgresDialog : public wxDialog {
public:
  explicit PostgresDialog(wxWindow* parent);
  void ConnectionString(const std::string& info);
  [[nodiscard]] std::string ConnectionString() const;

  [[nodiscard]] const std::string& DbName() const { return db_name_;}
  [[nodiscard]] const std::string& DbUser() const { return db_user_;}
  [[nodiscard]] bool HavePassword() const { return have_password_;}
private:
  wxString connection_string_;
  std::string db_name_; ///< Name of database in string.
  std::string db_user_; ///< User in string.
  bool have_password_ = false;
  void OnOk(wxCommandEvent& event);
  void OnTestConnection(wxCommandEvent& event);
  [[nodiscard]] bool CheckConnectionString();
  [[nodiscard]] bool TestConnection();
  wxDECLARE_EVENT_TABLE();
};

} // namespace ods::gui
