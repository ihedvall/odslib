/*
 * Copyright 2025 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <wx/docview.h>

#include "globalnamedoc.h"

namespace ods {

class GlobalNameView : public wxView {
 public:
  GlobalNameView() = default;
  void OnDraw(wxDC *dc) override;
 private:
 wxDECLARE_DYNAMIC_CLASS(GlobalNameView);

};

} // ods


