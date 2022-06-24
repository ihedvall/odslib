/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <ods/imodel.h>

namespace ods::gui {
class EnumDialog : public wxDialog {
 public:
  EnumDialog(wxWindow* parent, const IModel& model, const IEnum& enumerate);
  [[nodiscard]] const IEnum& GetEnum() const {
    return enumerate_;
  }
  void SetNameReadOnly(bool read_only = true) {
    if (enum_name_ctrl_ != nullptr) {
      enum_name_ctrl_->Enable(!read_only);
    }
  }
  bool TransferDataToWindow() override;
  bool TransferDataFromWindow() override;
 private:
  IEnum original_;
  IEnum enumerate_;
  const IModel& model_; ///< Reference to the model
  wxListView* item_list_ = nullptr;
  wxTextCtrl* enum_name_ctrl_ = nullptr;

  //Configuration
  wxString enum_name_;
  bool locked_ = true;

  void RedrawItemList();
  void OnRightClick(wxContextMenuEvent& event);
  void OnItemDoubleClick(wxListEvent& event);
  void OnItemSingleSelect(wxUpdateUIEvent& event);
  void OnItemSelect(wxUpdateUIEvent& event);
  void OnItemAdd(wxCommandEvent& event);
  void OnItemEdit(wxCommandEvent&);
  void OnItemDelete(wxCommandEvent& event);

  bool GetSelectedItem(int64_t& key, std::string& value) const;

 wxDECLARE_EVENT_TABLE();
};

} // end namespace




