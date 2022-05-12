/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "ods/ienvironment.h"

namespace ods {

IEnvironment::IEnvironment(EnvironmentType type)
: env_type_(type) {

}

} // end namespace