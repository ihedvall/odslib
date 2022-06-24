/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <unordered_set>
#include <array>
#include <wx/docmdi.h>
#include <wx/splitter.h>
#include "tablepanel.h"
#include "odsconfigid.h"
#include "tabledialog.h"
#include "columndialog.h"
#include "odsconfig.h"

namespace {

int BaseIdImage(ods::BaseId base_id) {
  switch (base_id) {
    case ods::BaseId::AoAny: return 0;
    case ods::BaseId::AoEnvironment: return 1;
    case ods::BaseId::AoSubTest: return 3;
    case ods::BaseId::AoMeasurement: return 4;
    case ods::BaseId::AoMeasurementQuantity: return 8;
    case ods::BaseId::AoQuantity: return 9;
    case ods::BaseId::AoQuantityGroup: return 10;
    case ods::BaseId::AoUnit: return 11;
    case ods::BaseId::AoUnitGroup: return 12;
    case ods::BaseId::AoPhysicalDimension: return 13;
    case ods::BaseId::AoUnitUnderTest: return 14;
    case ods::BaseId::AoUnitUnderTestPart: return 15;
    case ods::BaseId::AoTestEquipment: return 16;
    case ods::BaseId::AoTestEquipmentPart: return 17;
    case ods::BaseId::AoTestSequence: return 18;
    case ods::BaseId::AoTestSequencePart: return 19;
    case ods::BaseId::AoUser: return 20;
    case ods::BaseId::AoUserGroup: return 21;
    case ods::BaseId::AoTest: return 2;
    case ods::BaseId::AoTestDevice: return 17;
    case ods::BaseId::AoSubMatrix: return 5;
    case ods::BaseId::AoLocalColumn: return 6;
    case ods::BaseId::AoExternalComponent: return 7;
    case ods::BaseId::AoLog: return 22;
    case ods::BaseId::AoParameter: return 23;
    case ods::BaseId::AoParameterSet: return 24;
    case ods::BaseId::AoNameMap: return 26;
    case ods::BaseId::AoAttributeMap: return 27;
    case ods::BaseId::AoFile: return 28;
    case ods::BaseId::AoMimetypeMap: return 0;
    default:break;
  }
  return 30;
}

void AppendSubTable(wxTreeListCtrl& tree_list, wxTreeListItem& root, const ods::ITable& table) { // NOLINT
  const auto& table_list = table.SubTables();
  // Environment table is always the root. It's OK if it is missing, so we don't show it.
  for ( const auto& itr : table_list) {
    const auto& sub_table = itr.second;
    const auto bmp = BaseIdImage(sub_table.BaseId());
    auto item_root = tree_list.AppendItem(root,sub_table.ApplicationName(), bmp, bmp);
    tree_list.SetItemText(item_root, 1, BaseIdToUserText(sub_table.BaseId()));
    AppendSubTable(tree_list, item_root, sub_table);
  }
}

void SelectItem(wxTreeListCtrl& tree_list, const ods::IModel& model, int64_t app_id) {
  for (auto item = tree_list.GetFirstItem(); item.IsOk(); item = tree_list.GetNextItem(item)) {
    const auto name = tree_list.GetItemText(item).ToStdString();
    const auto* table = model.GetTableByName(name);
    if (table == nullptr || table->ApplicationId() != app_id) {
      continue;
    }
    tree_list.Select(item);
    break;
  }
}


} // end namespace

namespace ods::gui {

wxBEGIN_EVENT_TABLE(TablePanel, wxPanel) // NOLINT
        EVT_TREELIST_SELECTION_CHANGED(kIdTableList, TablePanel::OnTableSelect)
        EVT_TREELIST_ITEM_ACTIVATED(kIdTableList, TablePanel::OnTableActivated)
        EVT_TREELIST_ITEM_CONTEXT_MENU(kIdTableList, TablePanel::OnTableRightClick)
        EVT_COMMAND_CONTEXT_MENU(kIdColumnList, TablePanel::OnColumnRightClick)
        EVT_LIST_ITEM_ACTIVATED(kIdColumnList, TablePanel::OnColumnActivated)

        EVT_UPDATE_UI(kIdUniqueFlag, TablePanel::OnUpdateColumnSelected)
        EVT_UPDATE_UI(kIdNoUniqueFlag, TablePanel::OnUpdateColumnSelected)
        EVT_MENU(kIdUniqueFlag, TablePanel::OnUniqueFlag)
        EVT_MENU(kIdNoUniqueFlag, TablePanel::OnNoUniqueFlag)

        EVT_UPDATE_UI(kIdIndexFlag, TablePanel::OnUpdateColumnSelected)
        EVT_UPDATE_UI(kIdNoIndexFlag, TablePanel::OnUpdateColumnSelected)
        EVT_MENU(kIdIndexFlag, TablePanel::OnIndexFlag)
        EVT_MENU(kIdNoIndexFlag, TablePanel::OnNoIndexFlag)

        EVT_UPDATE_UI(kIdColumnUp, TablePanel::OnUpdateColumnSelected)
        EVT_UPDATE_UI(kIdColumnDown, TablePanel::OnUpdateColumnSelected)
        EVT_MENU(kIdColumnUp, TablePanel::OnColumnUp)
        EVT_MENU(kIdColumnDown, TablePanel::OnColumnDown)

        EVT_SIZE(TablePanel::OnSizeChange)

        EVT_BUTTON(kIdColumnUp, TablePanel::OnColumnUp)
        EVT_BUTTON(kIdColumnDown, TablePanel::OnColumnDown)

        EVT_UPDATE_UI(kIdCopyTable, TablePanel::OnUpdateTableSelected)
        EVT_UPDATE_UI(kIdPasteTable, TablePanel::OnUpdateCopyTableExist)
        EVT_MENU(kIdCopyTable, TablePanel::OnCopyTable)
        EVT_MENU(kIdPasteTable, TablePanel::OnPasteTable)
wxEND_EVENT_TABLE()

TablePanel::TablePanel(wxWindow *parent)
    : wxPanel(parent),
      image_list_(16, 16, false, 31),
      up_image_("UP", wxBITMAP_TYPE_BMP_RESOURCE),
      down_image_("DOWN", wxBITMAP_TYPE_BMP_RESOURCE) {
  image_list_.Add(wxBitmap("TREE_LIST", wxBITMAP_TYPE_BMP_RESOURCE));
  auto *splitter = new wxSplitterWindow(this);
  left_ = new wxTreeListCtrl(splitter, kIdTableList, wxDefaultPosition, {300, 600},
                             wxTL_SINGLE | wxTL_DEFAULT_STYLE);
  left_->AppendColumn("Name");
  left_->AppendColumn("Base Id");
  left_->SetImageList(&image_list_);

  auto* right_panel = new wxPanel(splitter);
  right_ = new wxListView(right_panel, kIdColumnList, wxDefaultPosition, {900, 600}, wxLC_REPORT);
  right_->AppendColumn("Flags", wxLIST_FORMAT_LEFT, 50);
  right_->AppendColumn("Name", wxLIST_FORMAT_LEFT, 100);
  right_->AppendColumn("DB", wxLIST_FORMAT_LEFT, 100);
  right_->AppendColumn("Base", wxLIST_FORMAT_LEFT, 100);
  right_->AppendColumn("Label", wxLIST_FORMAT_LEFT, 150);
  right_->AppendColumn("Type", wxLIST_FORMAT_LEFT,200);
  right_->AppendColumn("Reference", wxLIST_FORMAT_LEFT, 100);
  right_->AppendColumn("Description", wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE_USEHEADER);
  table_info_ = new wxStaticText(right_panel,wxID_ANY, wxEmptyString);
  auto font = table_info_->GetFont();
  font.MakeLarger();
  table_info_->SetFont(font);

  splitter->SplitVertically(left_, right_panel, 300);

  auto* right_sizer = new wxBoxSizer(wxVERTICAL);
  right_sizer->Add(table_info_, 0, wxALIGN_LEFT | wxALL | wxEXPAND , 0);
  right_sizer->Add(right_, 1, wxALIGN_LEFT | wxALL | wxEXPAND, 0);
  right_panel->SetSizerAndFit(right_sizer);

  auto* up_button = new wxBitmapButton(this, kIdColumnUp, up_image_);
  auto* down_button = new wxBitmapButton(this, kIdColumnDown, down_image_);
  auto* button_sizer = new wxBoxSizer(wxVERTICAL);
  button_sizer->Add(up_button, 0, wxALIGN_TOP | wxALL, 5);
  button_sizer->Add(down_button, 0, wxALIGN_TOP | wxALL, 5);

  auto *main_sizer = new wxBoxSizer(wxHORIZONTAL);
  main_sizer->Add(splitter, 1, wxALIGN_LEFT | wxALL | wxEXPAND, 0);
  main_sizer->Add(button_sizer, 0, wxALIGN_CENTER_VERTICAL| wxALL, 0);
  SetSizerAndFit(main_sizer);

  RedrawTableList();
  RedrawColumnList();
}

void TablePanel::RedrawTableList() {
  auto *doc = GetDocument();
  if (doc == nullptr || left_ == nullptr) {
    return;
  }

  const auto* selected = doc->GetSelectedTable();
  const int64_t selected_id = selected != nullptr ? selected->ApplicationId() : 0;

  left_->DeleteAllItems();
  const auto &model = doc->GetModel();
  const auto& table_list = model.Tables();

  constexpr std::array<BaseId,8> load_order = {BaseId::AoEnvironment,
                                               BaseId::AoTest,
                                               BaseId::AoUnitUnderTest,
                                               BaseId::AoTestSequence,
                                               BaseId::AoTestEquipment,
                                               BaseId::AoUnit,
                                               BaseId::AoPhysicalDimension,
                                               BaseId::AoQuantity };
  const auto root = left_->GetRootItem();
  for (const auto load : load_order) {
    for (const auto &itr: table_list) {
      const auto &table = itr.second;
      if (table.BaseId() != load) {
        continue;
      }
      const auto bmp = BaseIdImage(table.BaseId());
      auto item_root = left_->AppendItem(root, table.ApplicationName(), bmp, bmp);
      left_->SetItemText(item_root, 1, BaseIdToUserText(table.BaseId()));
      AppendSubTable(*left_, item_root, table);
    }
  }

  for (const auto &itr1: table_list) {
    const auto &table = itr1.second;
    const auto ignore = std::ranges::any_of(load_order, [&] (BaseId base) {
      return base == table.BaseId();
    });
    if (ignore) {
      continue;
    }
    const auto bmp = BaseIdImage(table.BaseId());
    auto item_root = left_->AppendItem(root, table.ApplicationName(), bmp, bmp);
    left_->SetItemText(item_root, 1, BaseIdToUserText(table.BaseId()));
    AppendSubTable(*left_, item_root, table);
  }

  if (selected_id > 0) {
    SelectItem(*left_, model, selected_id);
  }

}

void TablePanel::RedrawColumnList() {
  auto *doc = GetDocument();
  if (doc == nullptr || right_ == nullptr || table_info_ == nullptr) {
    return;
  }
  std::unordered_set<std::string> selected_list;
  for (auto item = right_->GetFirstSelected(); item >= 0; item = right_->GetNextSelected(item)) {
    selected_list.insert(right_->GetItemText(item,1).utf8_string());
  }
  right_->DeleteAllItems();
  const auto* table = doc->GetSelectedTable();

  if (table == nullptr) {
    table_info_->SetLabel("");
    return;
  }

  std::ostringstream info;
  info << table->ApplicationName() << " (" << table->ApplicationId() << ")";
  if (!table->DatabaseName().empty()) {
    info << ", DB: " << table->DatabaseName();
  }
  if (!table->Description().empty()) {
    info << ", Desc: " << table->Description();
  }
  table_info_->SetLabel(info.str());

  const auto& model = doc->GetModel();
  const auto& column_list = table->Columns();
  long row = 0;
  for (const auto& column : column_list) {
    std::ostringstream flags;
    if (column.Flags() & kUnique) {
      flags << "U";
    }
    if (column.Flags() & kObligatory) {
      flags << "O";
    }
    if (column.Flags() & kAutoGenerated) {
      flags << "A";
    }
    if (column.Flags() & kCaseSensitive) {
      flags << "C";
    }
    if (column.Flags() & kIndex) {
      flags << "I";
    }

    std::ostringstream type;
    type << DataTypeToUserText(column.DataType());
    if (column.DataType() == DataType::DtEnum) {
      type << " (" << column.EnumName() << ")";
    }

    if (!column.DefaultValue().empty()) {
      type << " '" << column.DefaultValue() << "'";
    }

    if (column.NofDecimals() >= 0 &&
       (column.DataType() == DataType::DtFloat || column.DataType() == DataType::DtDouble)) {
      type << " :" << column.NofDecimals();
    }

    if (column.DataLength() > 0) {
      type << " (" << column.DataLength() << ")";
    }
    if (!column.Unit().empty()) {
      type << " [" << column.Unit() << "]";
    }

    std::ostringstream ref;
    if (column.ReferenceId() > 0) {
      const auto* ref_table = model.GetTable(column.ReferenceId());
      if (ref_table != nullptr) {
        ref << ref_table->ApplicationName();
        const auto* ref_column = column.ReferenceName().empty() ?
            nullptr : ref_table->GetColumnByName(column.ReferenceName());
        if (ref_column != nullptr && !util::string::IEquals("id",ref_column->BaseName())) {
          ref << " (" << ref_column->ApplicationName() << ")";
        }
      }
    }


    right_->InsertItem(row, flags.str());
    right_->SetItem(row, 1, wxString::FromUTF8(column.ApplicationName()));
    right_->SetItem(row, 2, wxString::FromUTF8(column.DatabaseName()));
    right_->SetItem(row, 3, wxString::FromUTF8(column.BaseName()));
    right_->SetItem(row, 4, wxString::FromUTF8(column.DisplayName()));
    right_->SetItem(row, 5, wxString::FromUTF8(type.str()));
    right_->SetItem(row, 6, wxString::FromUTF8(ref.str()));
    right_->SetItem(row, 7, wxString::FromUTF8(column.Description()));
    if (selected_list.find(column.ApplicationName()) != selected_list.cend()) {
      right_->Select(row);
    }
    ++row;
  }
}

OdsDocument *TablePanel::GetDocument() const {
  const auto *child_frame = wxDynamicCast(GetGrandParent(), wxDocMDIChildFrame); // NOLINT
  return child_frame != nullptr ? wxDynamicCast(child_frame->GetDocument(), OdsDocument) : nullptr; //NOLINT
}

void TablePanel::Update() {
  RedrawTableList();
  RedrawColumnList();
  wxWindow::Update();
}

void TablePanel::OnTableSelect(wxTreeListEvent &event) {
  auto* doc = GetDocument();
  if (doc == nullptr || left_ == nullptr) {
    return;
  }

  const auto selected = event.GetItem();
  if (!selected.IsOk()) {
    doc->SelectTable(0);
  }

  const auto table_name = left_->GetItemText(selected).ToStdString();
  const auto& model = doc->GetModel();
  const auto* table = model.GetTableByName(table_name);
  doc->SelectTable(table == nullptr ? 0 : table->ApplicationId());

  RedrawColumnList();
}

void TablePanel::OnTableActivated(wxTreeListEvent&) {
  auto* doc = GetDocument();
  if (doc == nullptr) {
    return;
  }
  const auto* selected = doc->GetSelectedTable();
  if (selected == nullptr) {
    return;
  }

  TableDialog dialog(this, doc->GetModel(), *selected);
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }
  auto* update = const_cast<ITable*>(selected);
  if (update != nullptr) {
    *update = dialog.GetTable();
  }
  Update();
}

void TablePanel::OnTableRightClick(wxTreeListEvent &event) {
  wxMenu menu;
  menu.Append(kIdAddTable,wxGetStockLabel(wxID_ADD));
  menu.Append(kIdEditTable, wxGetStockLabel(wxID_EDIT));
  menu.Append(kIdDeleteTable, wxGetStockLabel(wxID_DELETE));
  menu.AppendSeparator();
  menu.Append(kIdCopyTable,wxGetStockLabel(wxID_COPY));
  menu.Append(kIdPasteTable, wxGetStockLabel(wxID_PASTE));
  PopupMenu(&menu);
}

void TablePanel::OnColumnRightClick(wxContextMenuEvent& event) {
  wxMenu menu;
  menu.Append(kIdAddColumn,wxGetStockLabel(wxID_ADD));
  menu.Append(kIdEditColumn, wxGetStockLabel(wxID_EDIT));
  menu.Append(kIdDeleteColumn, wxGetStockLabel(wxID_DELETE));
  menu.AppendSeparator();
  menu.Append(kIdUniqueFlag, L"Set Unique Column");
  menu.Append(kIdNoUniqueFlag, L"Unset Unique Column" );
  menu.AppendSeparator();
  menu.Append(kIdIndexFlag, L"Set Indexed Column");
  menu.Append(kIdNoIndexFlag, L"Unset Indexed Column" );
  menu.AppendSeparator();
  menu.Append(kIdColumnUp, L"Move Up");
  menu.Append(kIdColumnDown, L"Move Down" );
  PopupMenu(&menu);

}

void TablePanel::OnUpdateColumnSelected(wxUpdateUIEvent &event) {
  event.Enable(right_ != nullptr && right_->GetSelectedItemCount() > 0);
}

void TablePanel::OnUpdateSingleColumnSelected(wxUpdateUIEvent &event) {
  event.Enable(right_ != nullptr && right_->GetSelectedItemCount() == 1);
}

void TablePanel::OnAddColumn(wxCommandEvent &event) {
  auto* doc = GetDocument();
  if (doc == nullptr || right_ == nullptr) {
    return;
  }
  auto* table = doc->GetSelectedTable();
  if (table == nullptr) {
    return;
  }

  IColumn empty;
  empty.TableId(table->ApplicationId());
  empty.DataType(DataType::DtString);
  ColumnDialog dialog(this, doc->GetModel(), empty);
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }
  table->AddColumn(dialog.GetColumn());
  RedrawColumnList();
}

void TablePanel::OnEditColumn(wxCommandEvent &) {
  auto* doc = GetDocument();
  if (doc == nullptr) {
    return;
  }
  auto* table = doc->GetSelectedTable();
  if (table == nullptr || right_ == nullptr) {
    return;
  }
  auto* column = GetSelectedColumn();
  if (column == nullptr) {
    return;
  }
  ColumnDialog dialog(this, doc->GetModel(), *column);
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }
  const auto& after = dialog.GetColumn();
  if (util::string::IEquals(after.ApplicationName(), column->ApplicationName())) {
    *column = after;
  } else {
    std::ostringstream text;
    text << "The application name have been changed." << std::endl;
    text << "Do you want to create a new column or modify the existing one?";
    wxMessageDialog ask(this, text.str(), L"Create or Modify Column",
                        wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_QUESTION | wxCENTRE);
    ask.SetYesNoLabels(L"Create New", L"Modify");
    const auto ret1 = ask.ShowModal();
    if (ret1 == wxID_YES) {
      table->AddColumn(after);
    } else if (ret1 == wxID_NO) {
      *column = after;
    }
  }
  RedrawColumnList();
}

void TablePanel::OnDeleteColumn(wxCommandEvent &event) {
  auto* doc = GetDocument();
  if (doc == nullptr) {
    return;
  }
  auto* table = doc->GetSelectedTable();
  if (table == nullptr || right_ == nullptr) {
    return;
  }
  const auto& column_list = table->Columns();
  std::vector<std::string> del_list;
  for (auto item = right_->GetFirstSelected(); item >= 0; item = right_->GetNextSelected(item)) {
    if (item >= 0 || item < column_list.size()) {
      const auto& column = column_list[item];
      del_list.emplace_back(column.ApplicationName());
    }
  }
  if (del_list.empty()) {
    return;
  }

  std::ostringstream ask;
  ask << "Do you want to delete the following columns?";
  for (const auto& del : del_list) {
    ask << std::endl << "Column: " << del;
  }

  int ret = wxMessageBox(ask.str(), "Delete Column Dialog",
                         wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION,
                         this);

  if (ret != wxYES) {
    return;
  }
  for (const auto& del : del_list) {
    table->DeleteColumn(del);
  }
  RedrawColumnList();
}

void TablePanel::OnColumnActivated(wxListEvent &) {
  wxCommandEvent dummy;
  OnEditColumn(dummy);
}

IColumn *TablePanel::GetSelectedColumn() {
  auto* doc = GetDocument();
  if (doc == nullptr || right_ == nullptr) {
    return nullptr;
  }
  const auto* table = doc->GetSelectedTable();
  if (table == nullptr) {
    return nullptr;
  }

  const auto item = right_->GetFirstSelected();
  if (item < 0 || right_->GetSelectedItemCount() != 1) {
    return nullptr;
  }
  const auto* column = table->GetColumnByName(right_->GetItemText(item, 1).utf8_string());
  return column != nullptr ? const_cast<IColumn*>(column) : nullptr;
}

void TablePanel::OnUniqueFlag(wxCommandEvent&) {
  auto* doc = GetDocument();
  if (doc == nullptr || right_ == nullptr) {
    return;
  }
  auto* table = doc->GetSelectedTable();
  if (table == nullptr) {
    return;
  }
  for (auto item = right_->GetFirstSelected(); item >= 0; item = right_->GetNextSelected(item)) {
    const auto* column_c = table->GetColumnByName(right_->GetItemText(item, 1).utf8_string());
    auto* column = const_cast<IColumn*>(column_c);
    if (column != nullptr) {
      column->Unique(true);
    }
  }
  RedrawColumnList();
}

void TablePanel::OnNoUniqueFlag(wxCommandEvent&) {
  auto* doc = GetDocument();
  if (doc == nullptr || right_ == nullptr) {
    return;
  }
  auto* table = doc->GetSelectedTable();
  if (table == nullptr) {
    return;
  }
  for (auto item = right_->GetFirstSelected(); item >= 0; item = right_->GetNextSelected(item)) {
    const auto* column_c = table->GetColumnByName(right_->GetItemText(item, 1).utf8_string());
    auto* column = const_cast<IColumn*>(column_c);
    if (column != nullptr) {
      column->Unique(false);
    }
  }
  RedrawColumnList();
}

void TablePanel::OnIndexFlag(wxCommandEvent&) {
  auto* doc = GetDocument();
  if (doc == nullptr || right_ == nullptr) {
    return;
  }
  auto* table = doc->GetSelectedTable();
  if (table == nullptr) {
    return;
  }
  for (auto item = right_->GetFirstSelected(); item >= 0; item = right_->GetNextSelected(item)) {
    const auto* column_c = table->GetColumnByName(right_->GetItemText(item, 1).utf8_string());
    auto* column = const_cast<IColumn*>(column_c);
    if (column != nullptr) {
      column->Index(true);
    }
  }
  RedrawColumnList();
}

void TablePanel::OnNoIndexFlag(wxCommandEvent&) {
  auto* doc = GetDocument();
  if (doc == nullptr || right_ == nullptr) {
    return;
  }
  auto* table = doc->GetSelectedTable();
  if (table == nullptr) {
    return;
  }
  for (auto item = right_->GetFirstSelected(); item >= 0; item = right_->GetNextSelected(item)) {
    const auto* column_c = table->GetColumnByName(right_->GetItemText(item, 1).utf8_string());
    auto* column = const_cast<IColumn*>(column_c);
    if (column != nullptr) {
      column->Index(false);
    }
  }
  RedrawColumnList();
}

void TablePanel::OnColumnUp(wxCommandEvent&) {
  auto* doc = GetDocument();
  if (doc == nullptr || right_ == nullptr) {
    return;
  }
  auto* table = doc->GetSelectedTable();
  if (table == nullptr) {
    return;
  }
  auto& column_list = table->Columns();
  std::unordered_set<std::string> selected;
  for (auto item = right_->GetFirstSelected(); item >= 0; item = right_->GetNextSelected(item)) {
    selected.insert(right_->GetItemText(item,1).utf8_string());
  }

  for (auto itr = column_list.begin(); itr != column_list.end(); ++itr ) {
    std::string name = itr->ApplicationName();

    if (selected.find(name) != selected.cend() && itr != column_list.begin()) {
      auto prev = itr;
      --prev;
      IColumn temp = *itr;
      *itr = *prev;
      *prev = temp;
    }
  }
  RedrawColumnList();
}

void TablePanel::OnColumnDown(wxCommandEvent&) {
  auto* doc = GetDocument();
  if (doc == nullptr || right_ == nullptr) {
    return;
  }
  auto* table = doc->GetSelectedTable();
  if (table == nullptr) {
    return;
  }
  auto& column_list = table->Columns();
  std::unordered_set<std::string> selected;
  for (auto item = right_->GetFirstSelected(); item >= 0; item = right_->GetNextSelected(item)) {
    selected.insert(right_->GetItemText(item,1).utf8_string());
  }

  for (auto itr = column_list.rbegin(); itr != column_list.rend(); ++itr ) {
    std::string name = itr->ApplicationName();

    if (selected.find(name) != selected.cend() && itr != column_list.rbegin()) {
      auto prev = itr;
      --prev;
      IColumn temp = *itr;
      *itr = *prev;
      *prev = temp;
    }
  }
  RedrawColumnList();
}

void TablePanel::OnSizeChange(wxSizeEvent& event) {
  if (right_ != nullptr) {
    right_->SetColumnWidth(7,wxLIST_AUTOSIZE_USEHEADER);
  }
  event.Skip(true);
}

void TablePanel::OnUpdateTableSelected(wxUpdateUIEvent &event) {
  auto* doc = GetDocument();
  const ITable* table = doc != nullptr ? doc->GetSelectedTable() : nullptr;
  event.Enable(table != nullptr);
}

void TablePanel::OnUpdateCopyTableExist(wxUpdateUIEvent &event) {
  auto& app = wxGetApp();
  event.Enable(!app.CopyTable().ApplicationName().empty());
}

void TablePanel::OnCopyTable(wxCommandEvent& event) {
  auto* doc = GetDocument();
  const auto* selected = doc != nullptr ? doc->GetSelectedTable() : nullptr;
  auto& app = wxGetApp();
  if (selected != nullptr) {
    app.CopyTable(*selected);
  }
}

void TablePanel::OnPasteTable(wxCommandEvent& event) {
  auto* doc = GetDocument();
  if (doc == nullptr) {
    return;
  }
  auto& app = wxGetApp();
  const auto& table = app.CopyTable();
  if (table.ApplicationName().empty()) {
    return;
  }
  auto* selected = doc->GetSelectedTable();
  auto& model = doc->GetModel();

  ITable copy = table;
  if (selected != nullptr) {
    copy.ParentId(selected->ApplicationId());
  } else {
    copy.ParentId(0);
  }
  copy.ApplicationId(model.FindNextTableId(copy.ParentId()));
  std::string selected_name = copy.ApplicationName();

  const bool exist = model.GetTableByName(table.ApplicationName()) != nullptr;
  const bool have_sub_tables = !copy.SubTables().empty();
  if (exist && have_sub_tables) {
    std::ostringstream ask;
    ask << "The table already exist in the model. Table: " << copy.ApplicationName() << std::endl;
    ask << "Cannot add as it have sub-table!";
    const auto ret = wxMessageBox(ask.str(), L"Error Add Table", wxOK | wxCENTRE | wxICON_ERROR, this );
    return;
  }

  if (exist && !have_sub_tables) {
    std::ostringstream ask;
    ask << "The table already exist in the model. Table: " << copy.ApplicationName() << std::endl;
    ask << "Do you want to rename/modify the table?";
    const auto ret = wxMessageBox(ask.str(), "Modify Table Dialog", wxYES_NO | wxCENTRE | wxICON_WARNING, this );
    if (ret != wxYES) {
      return;
    }
    TableDialog dialog(this, model, copy);
    const auto ret1 = dialog.ShowModal();
    if (ret1 != wxID_OK) {
      return;
    }
    selected_name = dialog.GetTable().ApplicationName();
    if (model.GetTableByName(selected_name) == nullptr) {
      model.AddTable(dialog.GetTable());
    }
  } else {
    model.AddTable(copy);
  }
  const  ITable empty;
  app.CopyTable(empty); // Reset the copy table
  RedrawTableList();
  SelectTable(selected_name);
}

void TablePanel::SelectTable(const std::string& name) {
  auto* doc = GetDocument();
  if (doc == nullptr || left_ == nullptr) {
    return;
  }
  std::string selected_table;
  for (auto item = left_->GetFirstItem(); item.IsOk(); item = left_->GetNextItem(item)) {
    const auto table_name = left_->GetItemText(item).utf8_string();
    const bool match = util::string::IEquals(table_name, name);
    const bool selected = left_->IsSelected(item);
    if (match && !selected) {
      left_->Select(item);
      selected_table = table_name;
      left_->EnsureVisible(item);
    } else if (!match && selected) {
      left_->Unselect(item);
    }
  }
  const auto& model = doc->GetModel();
  const auto* table = model.GetTableByName(selected_table);
  doc->SelectTable(table == nullptr ? 0 : table->ApplicationId());
  RedrawColumnList();
}

} // end namespace