/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <wx/docview.h>
#include "odsdocument.h"

namespace ods::gui {
 class OdsView : public wxView  {
  public:
   OdsView() = default;

   bool OnCreate(wxDocument* doc, long flags) override;
   bool OnClose(bool del) override;

   void OnDraw(wxDC *dc) override;
   void OnUpdate(wxView *sender, wxObject *hint) override;

  private:

   wxDECLARE_DYNAMIC_CLASS(OdsView);

 };
}





