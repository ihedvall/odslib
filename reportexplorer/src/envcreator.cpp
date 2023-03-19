/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <wx/wx.h>
#include <wx/config.h>

#include <util/logstream.h>
#include "envcreator.h"
#include "testdirectory.h"
#include "reportexplorer.h"

using namespace util::log;

namespace ods::gui {

std::unique_ptr<IEnvironment> EnvCreator::CreateEnvironment(ods::EnvironmentType type) {
  std::unique_ptr<IEnvironment> env;
  auto& app = wxGetApp();
  switch (type) {
    case EnvironmentType::kTypeGeneric:
      break;

    case EnvironmentType::kTypeTestDirectory: {
      auto test_dir = std::make_unique<detail::TestDirectory>();
      test_dir->ModelFileName(app.ModelFileName());
      test_dir->DbFileName(app.DbFileName());
      env = std::move(test_dir);
      break;
    }

    default:
      break;
  }
  return env;
}
std::unique_ptr<IEnvironment> EnvCreator::CreateFromConfig(const std::string &name) {

  std::unique_ptr<IEnvironment> env;
  auto& app = wxGetApp();

  auto* config = wxConfig::Get();
  if (!config) {
    LOG_ERROR() << "Failed to get the configuration for the application.";
    return env;
  }
  const auto orig_path = config->GetPath();
  config->SetPath(name);

  const auto env_name = config->ReadObject("Name", wxString()).utf8_string();
  const auto type = TextToEnvType(config->ReadObject("Type", wxString()).utf8_string());
  const auto desc = config->ReadObject("Description", wxString()).utf8_string();
  const auto model_file = app.ModelFileName();

  switch (type) {
    case EnvironmentType::kTypeTestDirectory: {
      auto test_dir = std::make_unique<detail::TestDirectory>();
      test_dir->Name(env_name);
      test_dir->Description(desc);
      test_dir->ModelFileName(model_file);
      test_dir->RootDir(config->ReadObject("Root", wxString()).utf8_string());
      test_dir->DbFileName(app.DbFileName());
      test_dir->TestDirFormat(config->ReadObject("Format", wxString()).utf8_string());
      test_dir->TextToExcludeList(config->ReadObject("Exclude", wxString()).utf8_string());
      const auto init = test_dir->Init();
      if (!init) {
        LOG_ERROR() << "Failed to initialize the test directory environment. Environment: " << test_dir->Name();
      }
      env = std::move(test_dir);
      break;
    }
    default:
      break;
  }

  config->SetPath(orig_path);

  return env;
}

void EnvCreator::SaveToConfig(const IEnvironment *env) {
  if (env == nullptr || env->Name().empty()) {
    return;
  }
  auto* config = wxConfig::Get();
  if (!config) {
    LOG_ERROR() << "Failed to get the configuration for the application.";
    return;
  }
  const auto orig_path = config->GetPath();

  std::ostringstream group;
  group << "/EnvList/" << env->Name();
  config->SetPath(group.str());

  config->Write("Name", wxString::FromUTF8(env->Name()));
  config->Write("Type", wxString::FromUTF8(EnvTypeToText(env->Type())));
  config->Write("Description", wxString::FromUTF8(env->Description()));
  config->Write("Model", wxString::FromUTF8(env->ModelFileName()));
  switch (env->Type()) {
    case EnvironmentType::kTypeTestDirectory: {
      const auto* test_dir = dynamic_cast<const detail::TestDirectory*>(env);
      if (test_dir != nullptr) {
        config->Write("Root", wxString::FromUTF8(test_dir->RootDir()));
        config->Write("Database", wxString::FromUTF8(test_dir->DbFileName()));
        config->Write("Format", wxString::FromUTF8(test_dir->TestDirFormat()));
        config->Write("Exclude", wxString::FromUTF8(test_dir->ExcludeListToText()));
      }
      break;
    }
    default:
      break;
  }

  config->SetPath(orig_path);
}

} // end namespace