/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <sstream>
#include <filesystem>
#include <wx/msgdlg.h>
#include <util/logging.h>
#include "odsdocument.h"

namespace {

} // end namespace

namespace ods::gui {
wxIMPLEMENT_DYNAMIC_CLASS(OdsDocument, wxDocument) // NOLINT

wxBEGIN_EVENT_TABLE(OdsDocument, wxDocument) // NOLINT
  EVT_UPDATE_UI(wxID_SAVE, OdsDocument::OnUpdateSave)
wxEND_EVENT_TABLE()


void OdsDocument::OnUpdateSave(wxUpdateUIEvent &event) {
  try {
    std::filesystem::path file(GetFilename().ToStdString());
    if (!std::filesystem::exists(file)) {
      event.Enable(false);
      return;
    }
  } catch (const std::exception& ) {
    event.Enable(false);
    return;
  }
  event.Enable(IsModified());
}

OdsDocument::OdsDocument() {
  model_ = original_;
}

void OdsDocument::UpdateModified() {
  if (original_ == model_) {
    if (IsModified()) {
      Modify(false);
    }
  } else  {
    if (!IsModified()) {
      Modify(true);
    }
  }
}

ITable *OdsDocument::GetSelectedTable() {
  if (selected_table_ == 0) {
    return nullptr;
  }
  const auto* table = model_.GetTable(selected_table_);
  return table == nullptr ? nullptr : const_cast<ITable*>(table);
}

bool OdsDocument::DoSaveDocument(const wxString &file) {
  util::log::BackupFiles(file.ToStdString(), true);

  const auto save = model_.SaveModel(file.ToStdString());
  if (save) {
    original_ = model_;
  }
  return save;
}

bool OdsDocument::DoOpenDocument(const wxString &file) {
  wxBusyCursor wait;
  const auto read = model_.ReadModel(file.ToStdString());
  if (!read) {
    return false;
  }
  original_ = model_;
  SetFilename(file,true);
  return true;
}
} // namespace mdf::viewer