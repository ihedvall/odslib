/*
 * Copyright 2024 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <ods/imodel.h>

namespace ods::gui {



class RelationDialog : public wxDialog {
 public:
  RelationDialog(wxWindow* parent, const IModel& model, const IRelation& relation);
  [[nodiscard]] const IRelation& GetRelation() const {
    return relation_;
  }

  bool TransferDataToWindow() override;
  bool TransferDataFromWindow() override;

  void EndModal(int retCode) override;
 private:

  IRelation relation_;
  const IModel& model_; ///< Reference to the model

  wxTextCtrl* name_ctrl_ = nullptr; ///< The name control needed for focus control.
  wxTextCtrl* database_name_ctrl_ = nullptr; ///< The DB name control needed for focus control.
  //Configuration
  wxString name_;
  wxString table1_;
  wxString table2_;
  wxString database_name_;
  wxString inverse_name_;
  wxString base_name_;
  wxString inverse_base_name_;

  wxArrayString MakeTableList() const;

  wxDECLARE_EVENT_TABLE();

};

} // ods::gui


