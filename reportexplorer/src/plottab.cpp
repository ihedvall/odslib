/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include <set>
#include <vector>
#include <string>
#include <wx/textctrl.h>
#include <util/stringutil.h>
#include <ods/odsdef.h>
#include "plottab.h"
#include "reportexplorerid.h"
#include "reportexplorer.h"
#include "meastreedata.h"


using namespace util::string;

namespace {

constexpr std::string_view kMaxName = "XXXXXXXXXXXXXXXXX";


void RedrawUnitBox(wxComboBox& box) {
  const auto selection = box.GetValue().ToStdString();
  int selected = -1;
  box.Clear();
  box.SetEditable(true);
  box.Append("*");
  if (selection == "*") {
    selected = 0;
  }
  auto& app = ods::gui::wxGetApp();
  const auto& list = app.SelectedList();
  std::set<std::string> unit_list;
  for (const auto& itr : list) {
    const auto unit = itr->Value<std::string>("UnitName");
    if (unit_list.find(unit) == unit_list.cend()) {
      unit_list.insert(unit);
    }
  }
  int index = 1;
  for (const auto& text : unit_list) {
    if (text == selected) {
      selected = index;
    }
    box.Append(wxString::FromUTF8(text));
    ++index;
  }
  if (selected >= 0) {
    box.SetSelection(selected);
  }
  box.SetValue(selection);
}

void RedrawXAxisBox(wxComboBox& box) {
  const auto selection = box.GetValue().ToStdString();
  int selected = selection.empty() ? 0 : -1; // Sample is default for x-axis
  box.Clear();
  box.SetEditable(true);

  box.Append("Sample");
  if (selection == "Sample") {
    selected = 0;
  }

  auto& app = ods::gui::wxGetApp();
  auto& list = app.SelectedList();
  std::vector<ods::IItem*> independent_list;
  for (auto& itr : list) {
    if (!itr) {
      continue;
    }
    const auto ind = itr->Value<bool>("Independent");
    if (ind) {
      independent_list.push_back(itr.get());
    }
  }
  int index = 1;
  for (auto* item : independent_list) {
    if (item == nullptr) {
      continue;
    }
    std::ostringstream temp;
    temp << item->Name();
    if (independent_list.size() > 1) {
      temp << "/" << item->Value<std::string>("MeasName")
           << "/" << item->Value<std::string>("FileName")
           << "/" << item->Value<std::string>("TestName");
    }
    if (temp.str() == selection) {
      selected = index;
      item->SetAttribute({"Selected", "X"});
    }
    box.Append(wxString::FromUTF8(temp.str()));
    ++index;
  }
  if (selected >= 0) {
    box.SetSelection(selected);
  } else {
    box.SetValue(selection);
  }
}

ods::IItem* GetXItem(const std::string& name) {
  auto& app = ods::gui::wxGetApp();
  auto& list = app.SelectedList();
  ods::IItem* item = nullptr;
  for (auto& itr : list) {
    if (!itr) {
      continue;
    }
    if (IEquals(name, itr->Name())) {
     item = itr.get();
     itr->SetAttribute({"Selected", "X"});
     continue;
    }

    std::ostringstream temp;
    temp << itr->Name()
         << "/" << itr->Value<std::string>("MeasName")
         << "/" << itr->Value<std::string>("FileName")
         << "/" << itr->Value<std::string>("TestName");
    if (IEquals(name, temp.str())) {
      item = itr.get();
      itr->SetAttribute({"Selected", "X"});
    } else if (itr->Value<std::string>("Selected") == "X"){
      itr->SetAttribute({"Selected", "0"});
    }
  }
  return item;
}


}
namespace ods::gui {
wxBEGIN_EVENT_TABLE(PlotTab, wxPanel) // NOLINT
    EVT_COMBOBOX(kIdXAxis, PlotTab::OnSelectXAxis)
    EVT_TREELIST_ITEM_CONTEXT_MENU(kIdPlotList, PlotTab::OnContextMenu)
    EVT_UPDATE_UI_RANGE(kIdSelectX, kIdSelectY2, PlotTab::OnListNotEmpty)
    EVT_MENU(kIdSelectX, PlotTab::OnSelectX)
    EVT_MENU(kIdSelectY1, PlotTab::OnSelectY1)
    EVT_MENU(kIdSelectY2, PlotTab::OnSelectY2)
    EVT_BUTTON(kIdViewOnly, PlotTab::OnViewOnly)
wxEND_EVENT_TABLE()

PlotTab::PlotTab(wxWindow *parent)
    : wxPanel(parent),
      up_image_("UP", wxBITMAP_TYPE_BMP_RESOURCE),
      down_image_("DOWN", wxBITMAP_TYPE_BMP_RESOURCE) {
  list_ = new wxTreeListCtrl(this, kIdPlotList, wxDefaultPosition, {500, 400}, wxTL_SINGLE | wxTL_CHECKBOX);
  list_->AppendColumn(L"Channel",
                      wxCOL_WIDTH_AUTOSIZE,
                      wxALIGN_LEFT,
                      wxCOL_RESIZABLE | wxCOL_REORDERABLE | wxCOL_SORTABLE);
  list_->AppendColumn(L"Unit",
                      wxCOL_WIDTH_AUTOSIZE,
                      wxALIGN_LEFT,
                      wxCOL_RESIZABLE | wxCOL_REORDERABLE | wxCOL_SORTABLE);
  list_->AppendColumn(L"Axis",
                      wxCOL_WIDTH_AUTOSIZE,
                      wxALIGN_LEFT,
                      wxCOL_RESIZABLE | wxCOL_REORDERABLE | wxCOL_SORTABLE);
  list_->AppendColumn(L"Type",
                      wxCOL_WIDTH_AUTOSIZE,
                      wxALIGN_LEFT,
                      wxCOL_RESIZABLE | wxCOL_REORDERABLE | wxCOL_SORTABLE);
  list_->AppendColumn(L"Samples",
                      wxCOL_WIDTH_AUTOSIZE,
                      wxALIGN_LEFT,
                      wxCOL_RESIZABLE | wxCOL_REORDERABLE | wxCOL_SORTABLE);
  list_->AppendColumn(L"Test",
                      wxCOL_WIDTH_AUTOSIZE,
                      wxALIGN_LEFT,
                      wxCOL_RESIZABLE | wxCOL_REORDERABLE | wxCOL_SORTABLE);
  list_->AppendColumn(L"File",
                      wxCOL_WIDTH_AUTOSIZE,
                      wxALIGN_LEFT,
                      wxCOL_RESIZABLE | wxCOL_REORDERABLE | wxCOL_SORTABLE);
  list_->AppendColumn(L"Measurement",
                      wxCOL_WIDTH_AUTOSIZE,
                      wxALIGN_LEFT,
                      wxCOL_RESIZABLE | wxCOL_REORDERABLE | wxCOL_SORTABLE);
  list_->AppendColumn(L"Quantity",
                      wxCOL_WIDTH_AUTOSIZE,
                      wxALIGN_LEFT,
                      wxCOL_RESIZABLE | wxCOL_REORDERABLE | wxCOL_SORTABLE);
  list_->AppendColumn(L"Description",
                      wxCOL_WIDTH_AUTOSIZE,
                      wxALIGN_LEFT,
                      wxCOL_RESIZABLE | wxCOL_REORDERABLE | wxCOL_SORTABLE);
  list_->SetItemComparator(&comparator_);
  list_->SetSortColumn(0);

  // CHANNEL NAME FILTER
  channel_name_ = new wxSearchCtrl(this, kIdChannelNameFilter, kMaxName.data(), wxDefaultPosition, wxDefaultSize,
                                   wxTE_PROCESS_ENTER | wxTE_NOHIDESEL);
  channel_name_->ShowCancelButton(true);
  channel_name_->ShowSearchButton(true);
  channel_name_->SetValue(L"*");

  auto *name_label = new wxStaticText(this, wxID_ANY, L"Channel Name");
  auto *name_sizer = new wxBoxSizer(wxVERTICAL);
  name_sizer->Add(name_label, 0, wxALIGN_LEFT | wxALL, 1);
  name_sizer->Add(channel_name_, 0, wxALIGN_LEFT | wxALL, 1);

  // UNIT FILTER
  unit_ = new wxComboBox(this, kIdUnitFilter, "*", wxDefaultPosition, {100, -1},
                         0, nullptr, wxCB_DROPDOWN | wxTE_PROCESS_ENTER);

  auto *unit_label = new wxStaticText(this, wxID_ANY, L"Unit");
  auto *unit_sizer = new wxBoxSizer(wxVERTICAL);
  unit_sizer->Add(unit_label, 0, wxALIGN_LEFT | wxALL, 1);
  unit_sizer->Add(unit_, 0, wxALIGN_LEFT | wxALL, 1);

  auto *up_button = new wxBitmapButton(this, kIdChannelUp, up_image_);
  auto *down_button = new wxBitmapButton(this, kIdChannelDown, down_image_);

  auto *button_sizer = new wxBoxSizer(wxVERTICAL);
  button_sizer->Add(up_button, 0, wxALIGN_TOP | wxALL, 5);
  button_sizer->Add(down_button, 0, wxALIGN_TOP | wxALL, 5);

  auto *list_sizer = new wxBoxSizer(wxHORIZONTAL);
  list_sizer->Add(list_, 1, wxALL | wxEXPAND, 0);
  list_sizer->Add(button_sizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 0);
  // No need for channel sorting
  list_sizer->Hide(button_sizer, true);

  auto *filter_sizer = new wxBoxSizer(wxHORIZONTAL);
  filter_sizer->Add(name_sizer, 0, wxALL | wxEXPAND, 0);
  filter_sizer->Add(unit_sizer, 0, wxALL, 0);

  auto *filter_box = new wxStaticBoxSizer(wxHORIZONTAL, this, L"Filter");
  filter_box->Add(filter_sizer, 0, wxALL | wxEXPAND, 1);

  auto *view_button = new wxButton(this, kIdViewOnly, L"View Only");
  auto *view_create_button = new wxButton(this, kIdViewAndCreate, L"View and Create Report");
  auto *create_button = new wxButton(this, kIdCreateOnly, L"Create Report");

  auto *report_name = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_LEFT,
                                     wxTextValidator(wxFILTER_NONE, &report_name_));
  report_name->SetMinSize({20 * 10, -1});

  report_dir_ = new wxDirPickerCtrl(this, wxID_ANY, wxEmptyString, "Select Report Directory",
                                    wxDefaultPosition, wxDefaultSize,
                                    wxDIRP_DIR_MUST_EXIST | wxPB_USE_TEXTCTRL);
  report_dir_->SetMinSize({60 * 10, -1});

  x_axis_ = new wxComboBox(this, kIdXAxis, wxEmptyString, wxDefaultPosition, {40 * 10, -1},
                           0, nullptr, wxCB_DROPDOWN);

  script_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, {400, 300},
                           wxTE_LEFT | wxTE_MULTILINE | wxTE_READONLY);


  auto *report_name_label = new wxStaticText(this, wxID_ANY, L"Report Name:");
  auto *report_dir_label = new wxStaticText(this, wxID_ANY, L"Report Directory:");
  auto *x_axis_label = new wxStaticText(this, wxID_ANY, L"X Axis:");

  int label_width = 100;
  label_width = std::max(label_width, report_name_label->GetBestSize().GetX());
  label_width = std::max(label_width, report_dir_label->GetBestSize().GetX());
  label_width = std::max(label_width, x_axis_label->GetBestSize().GetX());

  report_name_label->SetMinSize({label_width, -1});
  report_dir_label->SetMinSize({label_width, -1});
  x_axis_label->SetMinSize({label_width, -1});

  auto *report_name_sizer = new wxBoxSizer(wxHORIZONTAL);
  report_name_sizer->Add(report_name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  report_name_sizer->Add(report_name, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto *report_dir_sizer = new wxBoxSizer(wxHORIZONTAL);
  report_dir_sizer->Add(report_dir_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  report_dir_sizer->Add(report_dir_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto *x_axis_sizer = new wxBoxSizer(wxHORIZONTAL);
  x_axis_sizer->Add(x_axis_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  x_axis_sizer->Add(x_axis_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto *command_sizer = new wxBoxSizer(wxVERTICAL);
  command_sizer->Add(view_button, 0, wxALL | wxEXPAND, 4);
  command_sizer->Add(view_create_button, 0, wxALL | wxEXPAND, 4);
  command_sizer->Add(create_button, 0, wxALL | wxEXPAND, 4);

  auto *edit_sizer = new wxBoxSizer(wxVERTICAL);
  edit_sizer->Add(report_name_sizer, 0, wxALL, 4);
  edit_sizer->Add(report_dir_sizer, 0, wxALL, 4);
  edit_sizer->Add(x_axis_sizer, 0, wxALL, 4);
  edit_sizer->Add(script_, 0, wxALL, 4);

  auto *plot_sizer = new wxBoxSizer(wxHORIZONTAL);
  plot_sizer->Add(edit_sizer, 0, wxALL | wxEXPAND, 0);
  plot_sizer->Add(command_sizer, 0, wxALL, 0);

  auto *main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(list_sizer, 1, wxALL | wxEXPAND, 0);
  main_sizer->Add(filter_box, 0, wxALL | wxEXPAND, 0);
  main_sizer->Add(plot_sizer, 0, wxALL | wxEXPAND, 0);

  SetSizerAndFit(main_sizer);
}
void PlotTab::Update() {
  wxWindow::Update();
  RedrawList();
  if (unit_ != nullptr) {
    RedrawUnitBox(*unit_);
  }
}

void PlotTab::RedrawList() {
  if (list_ == nullptr) {
    return;
  }
  if (x_axis_ != nullptr) {
    const auto x_sel = x_axis_->GetValue().ToStdString();
    GetXItem(x_sel);
  }

  auto &app = wxGetApp();
  const auto &selected_list = app.SelectedList();
  const auto root_item = list_->GetRootItem();
  list_->DeleteAllItems();
  for (const auto &channel: selected_list) {
    const auto index = channel->ItemId();
    const auto parent = channel->BaseValue<int64_t>("measurement");
    const auto independent = channel->Value<bool>("Independent");
    const auto selected = channel->Value<std::string>("Selected");
    const auto item = list_->AppendItem(root_item, wxString::FromUTF8(channel->Name()),
                                        wxTreeListCtrl::NO_IMAGE, wxTreeListCtrl::NO_IMAGE,
                                        new MeasTreeData(MeasTreeItem::ChannelItem, parent, index));
    list_->CheckItem(item, selected != "0" ? wxCHK_CHECKED : wxCHK_UNCHECKED);

    list_->SetItemText(item, 1, wxString::FromUTF8(channel->Value<std::string>("UnitName")));
    list_->SetItemText(item, 2, selected != "0" ? selected : "");
    list_->SetItemText(item, 3, DataTypeToUserText(static_cast<DataType>(channel->BaseValue<int>("datatype"))));
    list_->SetItemText(item, 4, std::to_string(channel->Value<size_t>("Samples")));
    list_->SetItemText(item, 5, wxString::FromUTF8(channel->Value<std::string>("TestName")));
    list_->SetItemText(item, 6, wxString::FromUTF8(channel->Value<std::string>("FileName")));
    list_->SetItemText(item, 7, wxString::FromUTF8(channel->Value<std::string>("MeasName")));
    list_->SetItemText(item, 8, wxString::FromUTF8(channel->Value<std::string>("QuantityName")));
    list_->SetItemText(item, 9, wxString::FromUTF8(channel->Value<std::string>("QuantityDesc")));
  }
  if (x_axis_ != nullptr) {
    RedrawXAxisBox(*x_axis_);
  }
  if (script_ != nullptr) {
    RedrawScript();
  }
}
void PlotTab::OnSelectXAxis(wxCommandEvent &event) {
  RedrawList();
}

void PlotTab::OnContextMenu(wxTreeListEvent &event) { // NOLINT
  wxMenu menu;

  menu.Append(kIdSelectY1, "Y1 Axis");
  menu.Append(kIdSelectY2, "Y2 Axis");
  menu.AppendSeparator();
  menu.Append(kIdSelectX, "X Axis");
  PopupMenu(&menu);
}

void PlotTab::OnListNotEmpty(wxUpdateUIEvent &event) {
  auto &app = wxGetApp();
  const auto &selected_list = app.SelectedList();
  event.Enable(!selected_list.empty() && list_ != nullptr);
}

void PlotTab::OnSelectX(wxCommandEvent &event) {
  if (list_ == nullptr) {
    return;
  }
  auto &app = ods::gui::wxGetApp();
  auto &list = app.SelectedList();
  const auto selected_item = list_->GetSelection();
  if (!selected_item.IsOk()) {
    return;
  }
  const auto *selected_data = list_->GetItemData(selected_item);
  if (selected_data == nullptr) {
    return;
  }
  const auto* data = dynamic_cast<const MeasTreeData*>(selected_data);
  if (data == nullptr) {
    return;
  }
  const auto index = data->Index();
  for (auto &itr: list) {
    if (!itr) {
      continue;
    }
    const auto sel = itr->Value<std::string>("Selected");
    if (itr->ItemId() == index) {
      itr->SetAttribute({"Selected", sel == "X" ? "0" : "X"});
    } else if (sel == "X") {
      itr->SetAttribute({"Selected", "0"});
    }
  }
  RedrawList();
}

void PlotTab::OnSelectY1(wxCommandEvent &event) {
  if (list_ == nullptr) {
    return;
  }
  auto &app = ods::gui::wxGetApp();
  auto &list = app.SelectedList();
  const auto selected_item = list_->GetSelection();
  if (!selected_item.IsOk()) {
    return;
  }
  const auto *selected_data = list_->GetItemData(selected_item);
  if (selected_data == nullptr) {
    return;
  }
  const auto* data = dynamic_cast<const MeasTreeData*>(selected_data);
  if (data == nullptr) {
    return;
  }
  const auto index = data->Index();
  auto* y_item = app.GetSelectedItem(index);
  if (y_item == nullptr) {
    return;
  }

  // Only selected Y1 items with the same unit is allowed
  const auto unit = y_item->BaseValue<int64_t>("unit");

  for (auto &itr: list) {
    if (!itr) {
      continue;
    }
    const auto sel = itr->Value<std::string>("Selected");
    const auto unit_index =itr->BaseValue<int64_t>("unit");

    if (itr->ItemId() == index) {
      itr->SetAttribute({"Selected", sel == "Y1" ? "0" : "Y1"});
    } else if (sel == "Y1" && unit_index != unit){
      itr->SetAttribute({"Selected", "0"});
    }
  }
  RedrawList();
}

void PlotTab::OnSelectY2(wxCommandEvent &event) {
  if (list_ == nullptr) {
    return;
  }
  auto &app = ods::gui::wxGetApp();
  auto &list = app.SelectedList();
  const auto selected_item = list_->GetSelection();
  if (!selected_item.IsOk()) {
    return;
  }
  const auto *selected_data = list_->GetItemData(selected_item);
  if (selected_data == nullptr) {
    return;
  }
  const auto* data = dynamic_cast<const MeasTreeData*>(selected_data);
  if (data == nullptr) {
    return;
  }
  const auto index = data->Index();
  auto* y_item = app.GetSelectedItem(index);
  if (y_item == nullptr) {
    return;
  }

  // Only selected Y1 items with the same unit is allowed
  const auto unit = y_item->BaseValue<int64_t>("unit");

  for (auto &itr: list) {
    if (!itr) {
      continue;
    }
    const auto sel = itr->Value<std::string>("Selected");
    const auto unit_index =itr->BaseValue<int64_t>("unit");

    if (itr->ItemId() == index) {
      itr->SetAttribute({"Selected", sel == "Y2" ? "0" : "Y2"});
    } else if (sel == "Y2" && unit_index != unit){
      itr->SetAttribute({"Selected", "0"});
    }
  }
  RedrawList();
}
/**
 * Generates a script text but doesn't read aor fills in data. The script text is for
 * viewing only. The script generators uses the CSV files header to plot
 * the X, Y1 and Y2 axes.
 */
void PlotTab::RedrawScript() {
  auto &app = ods::gui::wxGetApp();
  const auto &list = app.SelectedList();
  auto& plotter = app.Plotter();

  IItem* x_item = nullptr;
  std::vector<IItem*> y1_item_list;
  std::vector<IItem*> y2_item_list;
  for (const auto& itr : list) {
    const auto sel = itr->Value<std::string>("Selected");
    if (sel == "X") {
      x_item = itr.get();
    } else if (sel == "Y1") {
      y1_item_list.push_back(itr.get());
    } else if (sel == "Y2") {
      y2_item_list.push_back(itr.get());
    }
  }

  auto& csv = plotter.CsvFile();
  csv.Reset();
  if (x_item == nullptr) {
    csv.AddColumnHeader("Samples","");
  } else {
    csv.AddColumnHeader(x_item->Name(), x_item->Value<std::string>("UnitName"));
  }

  for (const auto* y1_item : y1_item_list) {
    if (y1_item == nullptr) {
      continue;
    }
    csv.AddColumnHeader(y1_item->Name(), y1_item->Value<std::string>("UnitName"));
  }

  for (const auto* y2_item : y2_item_list) {
    if (y2_item == nullptr) {
      continue;
    }
    csv.AddColumnHeader(y2_item->Name(), y2_item->Value<std::string>("UnitName"));
  }
  auto script = plotter.CreateScript();
  if (script_ != nullptr) {
    script_->SetValue(wxString::FromUTF8(script));
  }
}

void PlotTab::OnViewOnly(wxCommandEvent &event) {
  wxBusyCursor wait;
  auto& app = wxGetApp();
  app.PlotSelectedItem(false);
}


} // end namespace
