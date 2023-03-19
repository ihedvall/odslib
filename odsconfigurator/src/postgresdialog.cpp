/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include "postgresdialog.h"
#include <wx/config.h>
#include "odsconfigid.h"
#include <libpq-fe.h>
#include <sstream>
#include <util/stringutil.h>

using namespace util::string;

namespace {
  // If the connection string includes a password, the string is not
  // stored globally (registry on windows). Instead is it stored as
  // long the executable is running.
  std::string kInSessionString;
}

namespace ods::gui {

wxBEGIN_EVENT_TABLE(PostgresDialog, wxDialog) //NOLINT
 EVT_BUTTON(wxID_OK, PostgresDialog::OnOk)
 EVT_BUTTON(kIdTestConnection, PostgresDialog::OnTestConnection)
wxEND_EVENT_TABLE()

PostgresDialog::PostgresDialog(wxWindow *parent)
: wxDialog(parent, wxID_ANY,L"PostgreSQL Connection String",
               wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {
  if (auto* config = wxConfig::Get(); config != nullptr) {
     connection_string_= config->Read(L"/Postgres/ConnectionString", L"" );
  }
  if (!kInSessionString.empty()) {
    connection_string_ = wxString::FromUTF8(kInSessionString);
  }

  auto* label = new wxStaticText(this, wxID_ANY, L"Connection String:");
  auto* connection_string = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                                           wxDefaultPosition,wxDefaultSize, 0,
                           wxTextValidator(wxFILTER_NONE, &connection_string_));
  connection_string->SetMinSize({60*10, -1});

  auto* test_button = new wxButton(this, kIdTestConnection,
                                 L"Test Connection");
  auto* ok_button = new wxButton(this, wxID_OK,
                                  wxGetStockLabel(wxID_OK, wxSTOCK_FOR_BUTTON));
  auto* cancel_button = new wxButton(this, wxID_CANCEL,
                             wxGetStockLabel(wxID_CANCEL, wxSTOCK_FOR_BUTTON));

  auto* text_sizer = new wxBoxSizer(wxVERTICAL);
  text_sizer->Add(label, 0,  wxLEFT | wxRIGHT , 5);
  text_sizer->Add(connection_string, 0, wxLEFT | wxRIGHT , 5);
  text_sizer->Add(test_button, 0,wxALL, 5);

  auto* system_sizer = new wxStdDialogButtonSizer();
  system_sizer->AddButton(ok_button);
  system_sizer->AddButton(cancel_button);
  system_sizer->Realize();

  auto* main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(text_sizer, 0,  wxALIGN_LEFT | wxALL | wxEXPAND, 4);
  main_sizer->Add(system_sizer, 0,
                  wxALIGN_CENTER_HORIZONTAL | wxBOTTOM | wxLEFT | wxRIGHT, 10);

  SetSizerAndFit(main_sizer);
  ok_button->SetDefault();
}


void PostgresDialog::ConnectionString(const std::string &info) {
  connection_string_ = wxString::FromUTF8(info);
}

std::string PostgresDialog::ConnectionString() const {
  return connection_string_.ToStdString();
}

void PostgresDialog::OnOk(wxCommandEvent &event) {
  TransferDataFromWindow();
  const auto check = CheckConnectionString();
  if (!check) {
    event.Skip(false);
    return;
  }
  const auto test = TestConnection();
  if (!test) {
    event.Skip(false);
    return;
  }
  event.Skip(true);
}

void PostgresDialog::OnTestConnection(wxCommandEvent &event) {
  TransferDataFromWindow();
  const auto check = CheckConnectionString();
  if (!check) {
    return;
  }
  const auto test = TestConnection();
  if (!test) {
    return;
  }
  wxMessageBox(L"Connection successful", L"Connection OK",
               wxOK | wxICON_INFORMATION | wxCENTRE, this);
}

bool PostgresDialog::CheckConnectionString() {
  char *error_msg = nullptr;
  auto *connect_info = PQconninfoParse(ConnectionString().c_str(), &error_msg);
  if (error_msg != nullptr) {
    const std::string error = error_msg;
    free(error_msg);
    std::ostringstream msg;
    msg << "The connection string is invalid." << std::endl;
    msg << "String: " << ConnectionString() << std::endl;
    msg << "Error: " << error << std::endl;
    msg << "Do you want to continue using it anyway?";

    const int ret_val = wxMessageBox(wxString::From8BitData(msg.str().c_str()),
                               L"Invalid Connection String",
                               wxCENTRE | wxYES_NO | wxICON_ERROR | wxNO_DEFAULT,
                               this);
    if (ret_val == wxNO) {
      return false;
    }
  }
  have_password_ = false;
  db_name_.clear();
  db_user_.clear();
  for (size_t index = 0;
       connect_info != nullptr && connect_info[index].keyword != nullptr;
       ++index) {
    const auto &info = connect_info[index];
    if (info.val == nullptr) {
      continue;
    }

    if (IEquals(info.keyword, "password") && strlen(info.val) > 0) {
      have_password_ = true;
    }
    if (IEquals(info.keyword, "user") && strlen(info.val) > 0) {
      db_user_ = info.val;
    }
    if (IEquals(info.keyword, "dbname") && strlen(info.val) > 0) {
      db_name_ = info.val;
    }
  }
  PQconninfoFree(connect_info);
  if (have_password_) {
    kInSessionString = ConnectionString();
  } else if ( auto* config = wxConfig::Get();
             config != nullptr && !connection_string_.IsEmpty()) {
    config->Write(L"/Postgres/ConnectionString", connection_string_);
    config->Flush();
  }
  return true;
}

bool PostgresDialog::TestConnection() {

  if (auto* connect = PQconnectdb(ConnectionString().c_str());
      connect != nullptr) {
    const auto status = PQstatus(connect);
    if (status != CONNECTION_OK) {
      const auto* err_msg = PQerrorMessage(connect);
      std::string error = err_msg != nullptr ? err_msg : "";
      std::ostringstream msg;
      msg << "The test connection failed." << std::endl;
      msg << "String: " << ConnectionString() << std::endl;
      msg << "Error: " << error << std::endl;
      msg << "Do you want to continue anyway?";

      const int ret_val = wxMessageBox(wxString::From8BitData(msg.str().c_str()),
                                 L"Test Connection Failed",
                                 wxCENTRE | wxYES_NO | wxICON_ERROR | wxNO_DEFAULT,
                                 this);
      if (ret_val == wxNO) {
        PQfinish(connect);
        return false;
      }
      PQfinish(connect);
    }

  } else {
    const int ret_val = wxMessageBox(L"Test of connection failed.\nContinue anyway?",
                                     L"Test Connection Failed",
                                     wxCENTRE | wxYES_NO | wxICON_ERROR | wxNO_DEFAULT,
                                     this);
    if (ret_val == wxNO) {
      return false;
    }
  }
  return true;
}

} // namespace ods::gui