// ct_lvtcgn_codegendialog.h                                         -*-C++-*-

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

#ifndef _CT_LVTCGN_CODEGENDIALOG_H_INCLUDED
#define _CT_LVTCGN_CODEGENDIALOG_H_INCLUDED

#include <ct_lvtcgn_generatecode.h>
#include <lvtcgn_gui_export.h>
#include <ui_ct_lvtcgn_codegendialog.h>

#include <QDialog>
#include <QWidget>

#include <memory>
#include <string>
#include <vector>

namespace Codethink::lvtcgn::gui {

class LVTCGN_GUI_EXPORT CodeGenerationDialog : public QDialog {
  public:
    struct LVTCGN_GUI_EXPORT Detail {
        virtual ~Detail() = default;

        virtual QString getExistingDirectory(QDialog& dialog, QString const& defaultPath = QString());
        virtual void handleOutputDirEmpty(Ui::CodeGenerationDialogUi& ui);
        virtual void codeGenerationIterationCallback(Ui::CodeGenerationDialogUi& ui,
                                                     const mdl::CodeGeneration::CodeGenerationStep& step);
        virtual void handleCodeGenerationError(Ui::CodeGenerationDialogUi& ui,
                                               Codethink::lvtcgn::mdl::CodeGenerationError const& error);
        virtual void showErrorMessage(Ui::CodeGenerationDialogUi& ui, QString const& message);
        virtual QString executablePath() const;
    };

  private:
    struct Private;
    std::unique_ptr<Private> d;
    std::unique_ptr<Detail> impl;

  protected:
    void runCodeGeneration();
    void searchOutputDir();
    void populateAvailableScriptsCombobox();
    void openOutputDir() const;

  public:
    explicit CodeGenerationDialog(mdl::ICodeGenerationDataProvider& dataProvider,
                                  std::unique_ptr<Detail> impl = nullptr,
                                  QWidget *parent = nullptr);
    ~CodeGenerationDialog() override;

    void setOutputDir(const QString& dir);
    QString outputDir() const;
    QString selectedScriptPath() const;
};

} // namespace Codethink::lvtcgn::gui

#endif // CT_LVTCGN_CODEGENDIALOG_H_INCLUDED
