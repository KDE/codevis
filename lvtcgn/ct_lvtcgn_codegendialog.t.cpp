// ct_lvtcgn_codegendialog.t.cpp                                     -*-C++-*-

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

#include <ct_lvtcgn_codegendialog.h>
#include <ct_lvtcgn_generatecode.h>
#include <ct_lvtcgn_testutils.h>

#include <ct_lvttst_fixture_qt.h>
#include <ct_lvttst_tmpdir.h>

#include <QDebug>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeView>
#include <catch2/catch.hpp>

#pragma push_macro("slots")
#undef slots
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#pragma pop_macro("slots")

namespace py = pybind11;
struct PyDefaultGilReleasedContext {
    py::scoped_interpreter pyInterp;
    py::gil_scoped_release pyGilDefaultReleased;
};

static const std::string TMPDIR_NAME = "tmp_ct_lvtcgn_codegendialog";

using namespace Codethink::lvtcgn::gui;
using namespace Codethink::lvtcgn::mdl;

class MockedCodeGenerationDialogDetails : public CodeGenerationDialog::Detail {
  public:
    QString mockedOutputDir;
    QString mockedExecutablePath;
    int handleOutputDirEmptyCallCount = 0;
    int codeGenerationIterationCallCount = 0;
    int handleCodeGenerationErrorCallCount = 0;

  protected:
    QString getExistingDirectory(QDialog& dialog, QString const& defaultPath = "") override
    {
        (void) dialog;
        return mockedOutputDir;
    }

    void handleOutputDirEmpty(Ui::CodeGenerationDialogUi& ui) override
    {
        handleOutputDirEmptyCallCount += 1;
    }

    void codeGenerationIterationCallback(Ui::CodeGenerationDialogUi& ui,
                                         const CodeGeneration::CodeGenerationStep& step) override
    {
        (void) step;
        codeGenerationIterationCallCount += 1;
    }

    void handleCodeGenerationError(Ui::CodeGenerationDialogUi& ui,
                                   Codethink::lvtcgn::mdl::CodeGenerationError const& error) override
    {
        qDebug() << QString::fromStdString(error.message);
        handleCodeGenerationErrorCallCount += 1;
    }

    [[nodiscard]] QString executablePath() const override
    {
        return mockedExecutablePath;
    }
};

TEST_CASE_METHOD(QTApplicationFixture, "codegen dialog test")
{
    PyDefaultGilReleasedContext _pyDefaultGilReleasedContext;

    auto tmpDir = TmpDir{TMPDIR_NAME};
    auto outputDir = tmpDir.createDir("output");
    (void) tmpDir.createDir("python/codegeneration/testingscript/");

    auto contentProvider = FakeContentProvider{};
    auto dialogDetailPtr = std::make_unique<MockedCodeGenerationDialogDetails>();
    auto& dialogDetail = *dialogDetailPtr;
    dialogDetail.mockedExecutablePath = QString::fromStdString(tmpDir.path().string());
    auto dialog = CodeGenerationDialog{contentProvider, std::move(dialogDetailPtr)};

    auto *physicalEntitiesTree = dialog.findChild<QTreeView *>("physicalEntitiesTree");
    auto *treeModel = qobject_cast<QStandardItemModel *>(physicalEntitiesTree->model());
    auto getTextFrom = [&treeModel](QModelIndex const& i) {
        return treeModel->itemFromIndex(i)->text().toStdString();
    };
    auto isChecked = [&treeModel](QModelIndex const& i) {
        return treeModel->itemFromIndex(i)->checkState() == Qt::Checked;
    };

    REQUIRE(treeModel->rowCount() == 3);
    REQUIRE(getTextFrom(treeModel->index(0, 0)) == "somepkg_a");
    REQUIRE(getTextFrom(treeModel->index(1, 0)) == "somepkg_b");
    REQUIRE(getTextFrom(treeModel->index(2, 0)) == "somepkg_c");
    REQUIRE(getTextFrom(treeModel->index(0, 0, treeModel->index(2, 0))) == "component_a");
    REQUIRE(getTextFrom(treeModel->index(1, 0, treeModel->index(2, 0))) == "component_b");

    REQUIRE(isChecked(treeModel->index(0, 0)));
    REQUIRE(!isChecked(treeModel->index(1, 0)));
    REQUIRE(isChecked(treeModel->index(2, 0)));
    REQUIRE(!isChecked(treeModel->index(0, 0, treeModel->index(2, 0))));
    REQUIRE(isChecked(treeModel->index(1, 0, treeModel->index(2, 0))));

    auto testAndSetInputContents = [&dialog](const QString& inputFieldName,
                                             const QString& btnFieldName,
                                             QString& mockedRef,
                                             QString const& contents) {
        auto *input = dialog.findChild<QLineEdit *>(inputFieldName);
        auto *button = dialog.findChild<QPushButton *>(btnFieldName);
        REQUIRE(input != nullptr);
        REQUIRE(button != nullptr);

        mockedRef = contents;
        button->click();
        REQUIRE(input->text() == contents);

        // No return value means input will not be changed
        mockedRef = "";
        button->click();
        REQUIRE(input->text() == contents);
    };

    auto *runCodeGenerationBtn = dialog.findChild<QPushButton *>("runCodeGenerationBtn");
    REQUIRE(runCodeGenerationBtn != nullptr);

    REQUIRE(dialogDetail.handleOutputDirEmptyCallCount == 0);
    REQUIRE(dialogDetail.handleCodeGenerationErrorCallCount == 0);
    REQUIRE(dialogDetail.codeGenerationIterationCallCount == 0);

    runCodeGenerationBtn->click();

    REQUIRE(dialogDetail.handleOutputDirEmptyCallCount == 1);
    REQUIRE(dialogDetail.handleCodeGenerationErrorCallCount == 0);
    REQUIRE(dialogDetail.codeGenerationIterationCallCount == 0);

    testAndSetInputContents("outputDirInput",
                            "findOutputDirBtn",
                            dialogDetail.mockedOutputDir,
                            QString::fromStdString(outputDir.string()));
    {
        const std::string SCRIPT_CONTENTS =
            "\ndef buildPhysicalEntity(*args, **kwargs):"
            "\n    bad code!"
            "\n";
        (void) tmpDir.createTextFile("python/codegeneration/testingscript/codegenerator.py", SCRIPT_CONTENTS);
        runCodeGenerationBtn->click();
    }
    REQUIRE(dialogDetail.handleOutputDirEmptyCallCount == 1);
    REQUIRE(dialogDetail.handleCodeGenerationErrorCallCount == 1);
    REQUIRE(dialogDetail.codeGenerationIterationCallCount == 0);

    {
        const std::string SCRIPT_CONTENTS =
            "\ndef buildPhysicalEntity(*args, **kwargs):"
            "\n    pass"
            "\n";
        (void) tmpDir.createTextFile("python/codegeneration/testingscript/codegenerator.py", SCRIPT_CONTENTS);
        runCodeGenerationBtn->click();
    }
    REQUIRE(dialogDetail.handleOutputDirEmptyCallCount == 1);
    REQUIRE(dialogDetail.handleCodeGenerationErrorCallCount == 1);
    REQUIRE(dialogDetail.codeGenerationIterationCallCount == 3);
}

TEST_CASE_METHOD(QTApplicationFixture, "codegen find entities")
{
    PyDefaultGilReleasedContext _pyDefaultGilReleasedContext;

    auto tmp_dir = TmpDir{TMPDIR_NAME};
    auto outputDir = tmp_dir.createDir("output");

    auto contentProvider = FakeContentProvider{};
    auto dialogDetailPtr = std::make_unique<MockedCodeGenerationDialogDetails>();
    auto& dialogDetail = *dialogDetailPtr;
    dialogDetail.mockedExecutablePath = QString::fromStdString(tmp_dir.path().string());
    auto dialog = CodeGenerationDialog{contentProvider, std::move(dialogDetailPtr)};
    auto *physicalEntitiesTree = dialog.findChild<QTreeView *>("physicalEntitiesTree");
    auto *treeModel = qobject_cast<QStandardItemModel *>(physicalEntitiesTree->model());

    auto itemIsMarkedWithColor = [](QStandardItem *item, QString const& color) {
        return item->data(Qt::DisplayRole).toString().contains(color);
    };

    REQUIRE_FALSE(physicalEntitiesTree->isExpanded(treeModel->index(2, 0)));
    dialog.findChild<QLineEdit *>("searchValue")->setText("component");
    REQUIRE(physicalEntitiesTree->isExpanded(treeModel->index(2, 0)));
    auto *pgkItem = treeModel->itemFromIndex(treeModel->index(2, 0));
    REQUIRE(itemIsMarkedWithColor(pgkItem->child(0), "orange"));
    REQUIRE(itemIsMarkedWithColor(pgkItem->child(1), "yellow"));

    dialog.findChild<QPushButton *>("searchGoToNextBtn")->click();
    REQUIRE(itemIsMarkedWithColor(pgkItem->child(0), "yellow"));
    REQUIRE(itemIsMarkedWithColor(pgkItem->child(1), "orange"));

    dialog.findChild<QPushButton *>("searchGoToPrevBtn")->click();
    REQUIRE(itemIsMarkedWithColor(pgkItem->child(0), "orange"));
    REQUIRE(itemIsMarkedWithColor(pgkItem->child(1), "yellow"));

    dialog.findChild<QLineEdit *>("searchValue")->setText("pkg");
    REQUIRE_FALSE(itemIsMarkedWithColor(pgkItem->child(0), "yellow"));
    REQUIRE_FALSE(itemIsMarkedWithColor(pgkItem->child(0), "orange"));
    REQUIRE_FALSE(itemIsMarkedWithColor(pgkItem->child(1), "yellow"));
    REQUIRE_FALSE(itemIsMarkedWithColor(pgkItem->child(1), "orange"));
    REQUIRE(itemIsMarkedWithColor(pgkItem, "yellow"));
    dialog.findChild<QPushButton *>("searchGoToPrevBtn")->click();
    REQUIRE(itemIsMarkedWithColor(pgkItem, "orange"));
}

TEST_CASE_METHOD(QTApplicationFixture, "codegen selections")
{
    PyDefaultGilReleasedContext _pyDefaultGilReleasedContext;

    auto tmp_dir = TmpDir{TMPDIR_NAME};
    auto outputDir = tmp_dir.createDir("output");

    auto contentProvider = FakeContentProvider{};
    auto dialogDetailPtr = std::make_unique<MockedCodeGenerationDialogDetails>();
    auto& dialogDetail = *dialogDetailPtr;
    dialogDetail.mockedExecutablePath = QString::fromStdString(tmp_dir.path().string());
    auto dialog = CodeGenerationDialog{contentProvider, std::move(dialogDetailPtr)};
    auto *physicalEntitiesTree = dialog.findChild<QTreeView *>("physicalEntitiesTree");
    auto *treeModel = qobject_cast<QStandardItemModel *>(physicalEntitiesTree->model());

    auto requireState = [&treeModel](QModelIndex const& i, Qt::CheckState s) {
        REQUIRE(treeModel->itemFromIndex(i)->checkState() == s);
    };
    auto setState = [&treeModel](QModelIndex const& i, Qt::CheckState s) {
        treeModel->itemFromIndex(i)->setCheckState(s);
    };

    dialog.show();
    auto const& parent = treeModel->index(2, 0);
    physicalEntitiesTree->expand(parent);
    auto const& child1 = treeModel->index(0, 0, treeModel->index(2, 0));
    auto const& child2 = treeModel->index(1, 0, treeModel->index(2, 0));

    // Puts the tree in a known state before starting the test
    setState(parent, Qt::Unchecked);
    setState(child1, Qt::Unchecked);
    setState(child2, Qt::Unchecked);

    INFO("Selecting a parent item also updates the children")
    INFO("parent Checked")
    setState(parent, Qt::Checked);
    requireState(parent, Qt::Checked);
    requireState(child1, Qt::Checked);
    requireState(child2, Qt::Checked);

    INFO("parent Unchecked")
    setState(parent, Qt::Unchecked);
    requireState(parent, Qt::Unchecked);
    requireState(child1, Qt::Unchecked);
    requireState(child2, Qt::Unchecked);

    INFO("Selecting a child item also updates the parent")
    INFO("child1 Checked")
    setState(child1, Qt::Checked);
    requireState(parent, Qt::PartiallyChecked);
    requireState(child1, Qt::Checked);
    requireState(child2, Qt::Unchecked);
    INFO("child2 Checked")
    setState(child2, Qt::Checked);
    requireState(parent, Qt::Checked);
    requireState(child1, Qt::Checked);
    requireState(child2, Qt::Checked);

    INFO("child1 Unchecked")
    setState(child1, Qt::Unchecked);
    requireState(parent, Qt::PartiallyChecked);
    requireState(child1, Qt::Unchecked);
    requireState(child2, Qt::Checked);
    INFO("child2 Unchecked")
    setState(child2, Qt::Unchecked);
    requireState(parent, Qt::Unchecked);
    requireState(child1, Qt::Unchecked);
    requireState(child2, Qt::Unchecked);
}
