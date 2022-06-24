/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <string>
#include <wx/wx.h>
#include <ods/itable.h>
#include <ods/ienum.h>
#if !wxUSE_DOC_VIEW_ARCHITECTURE
#error You must set wxUSE_DOC_VIEW_ARCHITECTURE to 1 in setup.h!
#endif

namespace ods::gui {

class OdsConfig : public wxApp {
 public:

  bool OnInit() override;
  int OnExit() override;

  void OpenFile(const std::string& filename) const;

  [[nodiscard]] const ITable& CopyTable() const {
    return copy_table_;
  }
  void CopyTable(const ITable& table) {
    copy_table_ = table;
  }

  [[nodiscard]] const IEnum& CopyEnum() const {
    return copy_enum_;
  }
  void CopyEnum(const IEnum& enumerate) {
    copy_enum_ = enumerate;
  }

 private:
  std::string notepad_; ///< Path to notepad.exe if it exist.
  ITable copy_table_;   ///< Temporary storage of a table object and its column.
  IEnum copy_enum_;    ///< Temporary storage of an enumerate object and its items.

  void OnOpenLogFile(wxCommandEvent& event);
  void OnUpdateOpenLogFile(wxUpdateUIEvent& event);

  wxDECLARE_EVENT_TABLE();
};

wxDECLARE_APP(OdsConfig);
} // end namespace
