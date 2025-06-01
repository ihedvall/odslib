/*
 * Copyright 2025 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "globalnamedoc.h"

#include <filesystem>

#include <wx/filedlg.h>
#include <wx/config.h>

using namespace std::filesystem;

namespace ods {

wxIMPLEMENT_DYNAMIC_CLASS(GlobalNameDoc, wxDocument) // NOLINT

wxBEGIN_EVENT_TABLE(GlobalNameDoc, wxDocument) // NOLINT
wxEND_EVENT_TABLE()

GlobalNameDoc::GlobalNameDoc() {

}

bool GlobalNameDoc::OnNewDocument() {
  const bool new_doc = wxDocument::OnNewDocument();
  if (!new_doc) {
    return false;
  }
  const auto* doc_template = GetDocumentTemplate();
  if (doc_template == nullptr) {
    return false;
  }
  // Get the default directory
  wxString default_dir;
  if (auto* app_config = wxConfig::Get();
      app_config != nullptr ) {
    default_dir = app_config->Read("/General/DbPath" );
  }

  wxFileDialog file_dialog(wxApp::GetMainTopWindow(), wxString::FromUTF8("Select Database File"),
                       default_dir, wxEmptyString, doc_template->GetFileFilter(),
                       wxFD_OPEN);
  if (file_dialog.ShowModal() == wxID_CANCEL) {
    return false;
  }

  wxString database_file = file_dialog.GetPath();
  try {
    path filename(database_file.ToStdWstring());
    path db_path = filename.parent_path();
    if (auto* app_config = wxConfig::Get();
        app_config != nullptr ) {
      app_config->Write("/General/DbPath", wxString(db_path.generic_wstring()) );
    }
    if (exists(filename)) {
      return true;
    }
    // Create the database
  } catch (const std::exception& ) {
    return false;
  }
  return OnOpenDocument(database_file);
}

} // ods