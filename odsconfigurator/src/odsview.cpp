/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <wx/docmdi.h>
#include "odsview.h"
#include "odsconfig.h"
#include "childframe.h"

namespace ods::gui {

wxIMPLEMENT_DYNAMIC_CLASS(OdsView,wxView) //NOLINT

OdsDocument *OdsView::GetDocument() const {
  return wxDynamicCast(wxView::GetDocument(),OdsDocument );
}

void OdsView::OnDraw(wxDC*) {

}
bool OdsView::OnCreate(wxDocument *doc, long flags) {
  if (!wxView::OnCreate( doc,flags)) {
    return false;
  }

  auto & app = wxGetApp();
  auto* parent = wxDynamicCast(app.GetTopWindow(),wxMDIParentFrame);
  wxFrame* sub_frame = new ChildFrame(doc, this, parent,wxID_ANY,"MDF File");
  sub_frame->Show();
  return true;
}

bool OdsView::OnClose(bool del) {
  return wxView::OnClose(del);
}
void OdsView::OnUpdate(wxView *sender, wxObject *hint) {
  wxView::OnUpdate(sender, hint);
  auto* frame = GetFrame();
  if (frame != nullptr) {
    frame->Update();
  }

}

}