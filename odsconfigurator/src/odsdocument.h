/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <memory>
#include <wx/wx.h>
#include <wx/docview.h>
#include "ods/imodel.h"

namespace ods::gui {

class OdsDocument : public wxDocument {
 public:
  OdsDocument();
  ~OdsDocument() override = default;

  [[nodiscard]] IModel& GetModel() {
    return model_;
  }

  void SetModel(const IModel& model) {
    model_ = model;
    original_ = model;
  }

  ITable* GetSelectedTable();

  void UpdateModified();

  void SelectTable(int64_t application_id) {
    selected_table_ = application_id;
  }
 protected:
  bool DoSaveDocument(const wxString &file) override;
  bool DoOpenDocument(const wxString &file) override;
 private:
  ods::IModel original_;
  ods::IModel model_;
  int64_t selected_table_ = 0; ///< Application ID of the selected table

  void OnUpdateSave(wxUpdateUIEvent &event);
  wxDECLARE_DYNAMIC_CLASS(OdsDocument);
  wxDECLARE_EVENT_TABLE();
};



}



