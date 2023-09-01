// ct_lvtplg_handlercodeanalysis.h                                    -*-C++-*-

/*
// Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#ifndef DIAGRAM_SERVER_CT_LVTPLG_PLUGINCODEANALYSISHANDLER_H
#define DIAGRAM_SERVER_CT_LVTPLG_PLUGINCODEANALYSISHANDLER_H

#include <any>
#include <functional>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

struct PluginPhysicalParserOnHeaderFoundHandler {
    /**
     * Returns the plugin data previously registered with `PluginSetupHandler::registerPluginData`.
     */
    std::function<void *(std::string const& id)> const getPluginData;

    std::function<std::string()> const getSourceFile;
    std::function<std::string()> const getIncludedFile;
    std::function<unsigned()> const getLineNo;
};

struct PluginLogicalParserOnCppCommentFoundHandler {
    /**
     * Returns the plugin data previously registered with `PluginSetupHandler::registerPluginData`.
     */
    std::function<void *(std::string const& id)> const getPluginData;

    std::function<std::string()> const getFilename;
    std::function<std::string()> const getBriefText;
    std::function<unsigned()> const getStartLine;
    std::function<unsigned()> const getEndLine;
};

using RawDBData = std::optional<std::any>;
using RawDBCols = std::vector<RawDBData>;
using RawDBRows = std::vector<RawDBCols>;
struct PluginParseCompletedHandler {
    /**
     * Returns the plugin data previously registered with `PluginSetupHandler::registerPluginData`.
     */
    std::function<void *(std::string const& id)> const getPluginData;

    std::function<RawDBRows(std::string const& query)> const runQueryOnDatabase;
};

#endif // DIAGRAM_SERVER_CT_LVTPLG_PLUGINCODEANALYSISHANDLER_H
