/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <wx/wx.h>
#include "wx/filepicker.h"
#include "testdirectory.h"
namespace ods::gui {

class TestDirDialog : public wxDialog {
 public:
  TestDirDialog(wxWindow* parent, detail::TestDirectory& test_dir );

  bool TransferDataToWindow() override;
  bool TransferDataFromWindow() override;
 private:
  detail::TestDirectory& test_dir_;

  // IEnvironment
  wxString name_;
  wxString description_;
  wxFilePickerCtrl* model_file_ = nullptr;
  // Test Dir
  wxDirPickerCtrl* root_dir_ = nullptr;
  wxFilePickerCtrl* db_file_ = nullptr;
  wxString test_dir_format_;
  wxString exclude_list_;
 wxDECLARE_EVENT_TABLE();
};

} // end namespace




