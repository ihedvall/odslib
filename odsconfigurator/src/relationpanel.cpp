/*
 * Copyright 2024 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "relationpanel.h"
#include "odsconfigid.h"
#include <wx/docmdi.h>
#include "relationdialog.h"

namespace {
  constexpr int kBmpRelation = 5;


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

}

namespace ods::gui {

wxBEGIN_EVENT_TABLE(RelationPanel, wxPanel) //NOLINT
    EVT_LIST_ITEM_ACTIVATED(kIdRelationList, RelationPanel::OnDoubleClickRelation)
    EVT_CONTEXT_MENU( RelationPanel::OnRightClick)
wxEND_EVENT_TABLE()


RelationPanel::RelationPanel(wxWindow *parent)
    : wxPanel(parent),
      image_list_(16,16,false,31) {
  image_list_.Add(wxBitmap("TREE_LIST", wxBITMAP_TYPE_BMP_RESOURCE));
  list_ = new wxListView(this, kIdRelationList, wxDefaultPosition, {900, 600},
                         wxLC_REPORT | wxLC_SINGLE_SEL);
  list_->AppendColumn("Name", wxLIST_FORMAT_LEFT, 200);
  list_->AppendColumn("Table 1", wxLIST_FORMAT_LEFT, 150);
  list_->AppendColumn("Table 2", wxLIST_FORMAT_LEFT, 150);
  list_->AppendColumn("Database Name", wxLIST_FORMAT_LEFT, 100);
  list_->AppendColumn("Inverse Name", wxLIST_FORMAT_LEFT, 100);
  list_->AppendColumn("Base Name", wxLIST_FORMAT_LEFT, 100);
  list_->AppendColumn("Inverse Base Name", wxLIST_FORMAT_LEFT, 100);
  list_->SetImageList(&image_list_, wxIMAGE_LIST_SMALL);


  auto* main_sizer = new wxBoxSizer(wxHORIZONTAL);
  main_sizer->Add(list_, 1, wxALIGN_LEFT | wxALL | wxEXPAND, 4);
  SetSizerAndFit(main_sizer);

  RedrawRelationList();
}

void RelationPanel::RedrawRelationList() {
  auto *doc = GetDoc();
  if (doc == nullptr || list_ == nullptr) {
    return;
  }

  std::string selected_relation;
  auto selected = list_->GetFirstSelected();
  if (selected >= 0) {
    selected_relation = list_->GetItemText(selected, 0).ToStdString();
  }

  list_->DeleteAllItems();

  const auto &model = doc->GetModel();

  long row = 0;
  selected = -1;
  const auto& relation_list = model.GetRelationList();
  for (const auto& [name, relation] : relation_list) {
    if (!selected_relation.empty() && relation.Name() == selected_relation) {
      selected = row;
    }
    list_->InsertItem(row, wxString::FromUTF8(name), kBmpRelation);

    std::ostringstream text1;
    text1 << "(" << relation.ApplicationId1() << ")";
    BaseId base_id1 = BaseId::AoAny;
    const auto* table1 = model.GetTable(relation.ApplicationId1());
    if (table1 != nullptr) {
      text1 << " " << table1->ApplicationName();
      base_id1 = table1->BaseId();
    }
    list_->SetItem(row, 1, wxString::FromUTF8(text1.str()), BaseIdImage(base_id1));

    std::ostringstream text2;
    text2 << "(" << relation.ApplicationId2() << ")";
    BaseId base_id2 = BaseId::AoAny;
    const auto* table2 = model.GetTable(relation.ApplicationId2());
    if (table2 != nullptr) {
      text2 << " " << table2->ApplicationName();
      base_id2 = table2->BaseId();
    }
    list_->SetItem(row, 2, wxString::FromUTF8(text2.str()), BaseIdImage(base_id2));

    list_->SetItem(row, 3, wxString::FromUTF8( relation.DatabaseName() ));
    list_->SetItem(row, 4, wxString::FromUTF8( relation.InverseName() ));
    list_->SetItem(row, 5, wxString::FromUTF8( relation.BaseName() ));
    list_->SetItem(row, 6, wxString::FromUTF8( relation.InverseBaseName() ));

    ++row;
  }

  if (selected >= 0) {
    list_->Select(selected);
    list_->EnsureVisible(selected);
  }
}

OdsDocument *RelationPanel::GetDoc() const {
  const auto *child_frame = wxDynamicCast(GetGrandParent(), wxDocMDIChildFrame); // NOLINT
  return child_frame != nullptr ? wxDynamicCast(child_frame->GetDocument(), OdsDocument) : nullptr; //NOLINT
}

void RelationPanel::Update() {
  RedrawRelationList();
  wxWindow::Update();
}

void RelationPanel::OnRightClick(wxContextMenuEvent& event) {
  switch (event.GetId()) {
    case kIdRelationList: {
      wxMenu menu;
      menu.Append(kIdAddRelation,wxGetStockLabel(wxID_ADD));
      menu.Append(kIdEditRelation, wxGetStockLabel(wxID_EDIT));
      menu.Append(kIdDeleteRelation, wxGetStockLabel(wxID_DELETE));
      PopupMenu(&menu);
      break;
    }

    default:
      break;
  }

}

void RelationPanel::OnUpdateSingleRelationSelected(wxUpdateUIEvent &event) {
  if (list_ == nullptr) {
    event.Enable(false);
    return;
  }
  event.Enable(list_->GetSelectedItemCount() == 1);
}

void RelationPanel::OnUpdateRelationSelected(wxUpdateUIEvent &event) {
  if (list_ == nullptr) {
    event.Enable(false);
    return;
  }
  event.Enable(list_->GetSelectedItemCount() > 0);
}

void RelationPanel::OnAddRelation(wxCommandEvent &) {
  auto* doc = GetDoc();
  if (doc == nullptr) {
    return;
  }
  auto& model = doc->GetModel();
  // Normally the selected relation should be copied here but
  // most properties should be unique for this relation.
  const IRelation empty;
  RelationDialog dialog(this, model, empty);
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }
  const IRelation& relation = dialog.GetRelation();
  if (relation.Name().empty() ) {
    std::ostringstream message;
    message << "The many-to-many relation table cannot have an empty reference name." << std::endl;
    message << "Error: Reference Name is an empty string.";
    wxMessageDialog ask(this, wxString::FromUTF8(message.str()), L"Invalid Name",
                        wxOK | wxOK_DEFAULT | wxICON_ERROR | wxCENTRE);
    ask.ShowModal();
    return;
  }

  model.AddRelation(dialog.GetRelation());
  RedrawRelationList();
  SelectRelation(dialog.GetRelation().Name());
}

void RelationPanel::OnEditRelation(wxCommandEvent &) {
  auto* doc = GetDoc();
  auto* selected_relation = GetSelectedRelation();
  if (selected_relation == nullptr || doc == nullptr) {
    return;
  }
  auto& model = doc->GetModel();
  RelationDialog dialog(this, model, *selected_relation);
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }
  const auto& after = dialog.GetRelation();
  if (selected_relation->Name() != after.Name()) {
    wxMessageDialog ask(this,
                        L"The many-to-many relation name has changed.\nDo you want to create a new or modify the existing one? ",
                        L"Create or Modify Many-to-Many Relation", wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_QUESTION | wxCENTRE);
    ask.SetYesNoLabels(L"Create New", L"Modify");
    const auto ret1 = ask.ShowModal();
    if (ret1 == wxID_YES) {
      model.AddRelation(after);
    } else if (ret1 == wxID_NO) {
      model.DeleteRelation(selected_relation->Name());
      model.AddRelation(after);
    }
  } else {
    *selected_relation = after;
  }
  RedrawRelationList();
}

void RelationPanel::OnDeleteRelation(wxCommandEvent &) {
  auto* doc = GetDoc();
  if (list_ == nullptr || doc == nullptr) {
    return;
  }

  std::vector<std::string> del_list;
  for (auto item = list_->GetFirstSelected(); item >= 0; item = list_->GetNextSelected(item)) {
    std::string value = list_->GetItemText(item).utf8_string();
    del_list.emplace_back(value);
  }
  if (del_list.empty()) {
    return;
  }

  std::ostringstream ask;
  ask << "Do you want to delete the following many-to-many relations?";
  for (const auto& del : del_list) {
    ask << std::endl << del;
  }

  const int ret = wxMessageBox(ask.str(), "Delete Many-to-Many Relation",
                         wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION,
                         this);

  if (ret != wxYES) {
    return;
  }
  auto& model = doc->GetModel();
  for (const auto& del : del_list) {
    model.DeleteRelation(del);
  }
  RedrawRelationList();
}

void RelationPanel::OnDoubleClickRelation(wxListEvent &) {
  wxCommandEvent dummy;
  OnEditRelation(dummy);
}


IRelation *RelationPanel::GetSelectedRelation() {
  auto* doc = GetDoc();
  if (doc == nullptr || list_ == nullptr) {
    return nullptr;
  }
  const auto& model = doc->GetModel();

  const auto item = list_->GetFirstSelected();
  if (item < 0 || list_->GetSelectedItemCount() > 1) {
    return nullptr;
  }
  const auto name = list_->GetItemText(item).ToStdString();
  const auto* relation = model.GetRelationByName(name);
  if (relation == nullptr) {
    return nullptr;
  }
  return const_cast<IRelation*>(relation);
}

void RelationPanel::SelectRelation(const std::string& name) {
  if (list_ == nullptr) {
    return;
  }

  for (long item = list_->GetNextItem(-1) ; item >= 0; item = list_->GetNextItem(item)) {
    const auto relation_name = list_->GetItemText(item).utf8_string();
    const bool match = util::string::IEquals(relation_name, name);
    const bool selected = list_->IsSelected(item);
    if (match && !selected) {
      list_->Select(item);
      list_->EnsureVisible(item);
    } else if (!match && selected) {
      list_->Select(item, false);
    }
  }
}

} // ods