// ct_lvtcgn_generatecode.h                                       -*-C++-*-

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

#ifndef CT_LVTCGN_CODEGEN_H_INCLUDED
#define CT_LVTCGN_CODEGEN_H_INCLUDED

#include <lvtcgn_mdl_export.h>

#include <result/result.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Codethink::lvtcgn::mdl {

class LVTCGN_MDL_EXPORT IPhysicalEntityInfo {
    /**
     * Implementations of this class are meant to be a thin, cheaply copiable layer that mostly
     * dispatches the calls to an actual underlying model, although having primitive data should be
     * fine, as long as it remains cheap to copy.
     *
     * The reason for that is that we don't have currently a way to avoid direct dependency to database
     * data structures.
     *
     * TODO [#437]: Verify if this interface still makes sense AFTER architecture review.
     * _Maybe_ we can remove the thin-layer implementations when task #437 is done.
     *
     */
  public:
    virtual ~IPhysicalEntityInfo();
    virtual std::string name() const = 0;
    virtual std::string type() const = 0;
    virtual std::optional<std::reference_wrapper<IPhysicalEntityInfo>> parent() const = 0;
    virtual std::vector<std::reference_wrapper<IPhysicalEntityInfo>> children() const = 0;
    virtual std::vector<std::reference_wrapper<IPhysicalEntityInfo>> fwdDependencies() const = 0;
    virtual bool selectedForCodeGeneration() const = 0;
    virtual void setSelectedForCodeGeneration(bool value) = 0;
};

class LVTCGN_MDL_EXPORT ICodeGenerationDataProvider {
  public:
    virtual ~ICodeGenerationDataProvider();
    virtual std::vector<std::reference_wrapper<IPhysicalEntityInfo>> topLevelEntities() = 0;
    virtual int numberOfPhysicalEntities() const = 0;
};

struct LVTCGN_MDL_EXPORT CodeGenerationError {
    enum class Kind { PythonError, ScriptDefinitionError };

    Kind kind;
    std::string message;
};

class LVTCGN_MDL_EXPORT CodeGeneration {
  public:
    class CodeGenerationStep {
      public:
        virtual ~CodeGenerationStep() = default;
    };

    class ProcessEntityStep : public CodeGenerationStep {
      public:
        explicit ProcessEntityStep(const std::string& entityName): m_entityName(entityName)
        {
        }

        std::string entityName() const
        {
            return m_entityName;
        }

      private:
        std::string m_entityName;
    };

    class BeforeProcessEntitiesStep : public CodeGenerationStep { };
    class AfterProcessEntitiesStep : public CodeGenerationStep { };

    static cpp::result<void, CodeGenerationError>
    generateCodeFromScript(const std::string& scriptPath,
                           const std::string& outputDir,
                           ICodeGenerationDataProvider& dataProvider,
                           std::optional<std::function<void(CodeGenerationStep const&)>> callback = std::nullopt);
};

} // namespace Codethink::lvtcgn::mdl

#endif // CT_LVTCGN_CODEGEN_H_INCLUDED
