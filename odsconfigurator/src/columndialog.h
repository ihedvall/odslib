/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <wx/wx.h>
#include <ods/imodel.h>

namespace ods::gui {

class ColumnDialog : public wxDialog {
 public:
  ColumnDialog(wxWindow* parent, IModel& model, const IColumn& column);
  [[nodiscard]] const IColumn& GetColumn() const {
    return column_;
  }
  bool TransferDataToWindow() override;
  bool TransferDataFromWindow() override;
 private:
  IColumn column_;
  IModel& model_; ///< Reference to the model

  // Column Information
  wxString application_name_;
  wxString database_name_;
  wxString display_name_;
  wxString base_name_;
  wxString description_;

  // Reference Table/Column
  wxString reference_table_;
  wxString reference_column_;

  // Data Type
  wxString data_type_;
  wxString enum_name_;
  uint16_t length_ = 0;
  int      nof_decimals_ = -1;
  wxString unit_;
  wxString default_value_;

  // Flags
  bool auto_ = false;
  bool unique_ = false;
  bool obligatory_ = false;
  bool case_sensitive_ = false;
  bool index_ = false;



  bool ValidateColumn();
  wxDECLARE_EVENT_TABLE();
};

} // end namespace



