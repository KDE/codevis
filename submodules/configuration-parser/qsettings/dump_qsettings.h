/*
    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>
    SPDX-License-Identifier: MIT
*/

#pragma once

#include <parser/meta_settings.h>

#include <string>

namespace QSettingsExport {

void dump_header(
    const MetaConfiguration &config,
    const std::string &filename,
    const std::string &exportHeader);

void dump_source(
    const MetaConfiguration &config,
    const std::string &filename);

}
