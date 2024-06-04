// ct_lvtcgn_codegendialog.cpp                                       -*-C++-*-

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
#include <ct_lvtcgn_cogedentreemodel.h>

#include <QDesktopServices>
#include <QFileDialog>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QWidget>
#include <filesystem>
#include <kwidgetsaddons_version.h>
#include <memory>

namespace Codethink::lvtcgn::gui {

using namespace Codethink::lvtcgn::mdl;

class FindOnTreeModel : public QObject {
  public:
    void setTreeView(QTreeView *t)
    {
        treeView = t;
    }

    void findText(QString const& text)
    {
        auto *treeModel = dynamic_cast<CodeGenerationEntitiesTreeModel *>(treeView->model());

        currentNeedle = text;
        foundItems.clear();
        currentFindItem = foundItems.end();

        if (currentNeedle.isEmpty()) {
            treeModel->recursiveExec([&](QStandardItem *item) {
                auto entityName = item->data(CodeGenerationDataRole::EntityNameRole).toString();
                item->setData(entityName, Qt::DisplayRole);
                return CodeGenerationEntitiesTreeModel::RecursiveExec::ContinueSearch;
            });
            return;
        }

        // cppcheck things we can move 'firstFoundItem' to inside the lambda, but this would change the intended
        // behavior and introduce a bug, so a 'cppcheck-suppress' has been added below.
        auto firstFoundItem = true; // cppcheck-suppress variableScope
        treeModel->recursiveExec([&](QStandardItem *item) {
            auto entityName = item->data(CodeGenerationDataRole::EntityNameRole).toString();
            if (entityName.contains(currentNeedle)) {
                highlightNeedleOnItem(item, currentNeedle, FOUND_COLOR);
                foundItems.push_back(item);
                currentFindItem = foundItems.begin();
                if (firstFoundItem) {
                    goToItem(*currentFindItem, nullptr);
                    firstFoundItem = false;
                }
            } else {
                item->setData(entityName, Qt::DisplayRole);
            }
            return CodeGenerationEntitiesTreeModel::RecursiveExec::ContinueSearch;
        });
    }

    void goToNextItem()
    {
        if (foundItems.empty()) {
            return;
        }

        auto prevFindItem = currentFindItem;
        if (currentFindItem + 1 >= foundItems.end()) {
            currentFindItem = foundItems.begin();
        } else {
            currentFindItem++;
        }

        goToItem(*currentFindItem, *prevFindItem);
    }

    void goToPrevItem()
    {
        if (foundItems.empty()) {
            return;
        }

        auto prevFindItem = currentFindItem;
        if (currentFindItem - 1 < foundItems.begin()) {
            currentFindItem = foundItems.end() - 1;
        } else {
            currentFindItem--;
        }

        goToItem(*currentFindItem, *prevFindItem);
    };

  private:
    void recursivelyExpandItem(QStandardItem *item)
    {
        while (item) {
            treeView->setExpanded(item->index(), true);
            item = item->parent();
        }
    }

    static void highlightNeedleOnItem(QStandardItem *item, QString const& needle, QString const& color)
    {
        auto highlightedText = item->data(CodeGenerationDataRole::EntityNameRole).toString();
        highlightedText.replace(needle, "<span style='background-color: " + color + "'>" + needle + "</span>");
        item->setData(highlightedText, Qt::DisplayRole);
    }

    void goToItem(QStandardItem *currentItem, QStandardItem *prevItem)
    {
        recursivelyExpandItem(currentItem);
        treeView->scrollTo(currentItem->index());
        if (prevItem) {
            highlightNeedleOnItem(prevItem, currentNeedle, FOUND_COLOR);
        }
        highlightNeedleOnItem(currentItem, currentNeedle, SELECTED_COLOR);
    }

    QTreeView *treeView = nullptr;
    std::vector<QStandardItem *> foundItems;
    decltype(foundItems)::iterator currentFindItem = foundItems.end();
    QString currentNeedle;

    static auto constexpr SELECTED_COLOR = "orange";
    static auto constexpr FOUND_COLOR = "yellow";
};

struct CodeGenerationDialog::Private {
    Ui::CodeGenerationDialogUi ui;
    CodeGenerationEntitiesTreeModel treeModel;
    ICodeGenerationDataProvider& dataProvider;
    FindOnTreeModel findOnTreeModel;

    explicit Private(ICodeGenerationDataProvider& dataProvider):
        ui{}, treeModel(dataProvider), dataProvider(dataProvider)
    {
    }
};

QString CodeGenerationDialog::Detail::getExistingDirectory(QDialog& dialog, QString const& defaultPath)
{
    auto selectedPath = defaultPath.isEmpty() ? QDir::currentPath() : defaultPath;
    return QFileDialog::getExistingDirectory(&dialog, tr("Output directory"), selectedPath);
}

void CodeGenerationDialog::Detail::handleOutputDirEmpty(Ui::CodeGenerationDialogUi& ui)
{
    showErrorMessage(ui, tr("Please select the output directory."));
}

void CodeGenerationDialog::Detail::showErrorMessage(Ui::CodeGenerationDialogUi& ui, QString const& message)
{
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
    ui.topMessageWidget->clearActions();
#else
    const auto ourActions = ui.topMessageWidget->actions();
    for (auto *action : ourActions) {
        ui.topMessageWidget->removeAction(action);
    }
#endif
    ui.topMessageWidget->setText(message);
    ui.topMessageWidget->animatedShow();
}

void CodeGenerationDialog::Detail::codeGenerationIterationCallback(Ui::CodeGenerationDialogUi& ui,
                                                                   const mdl::CodeGeneration::CodeGenerationStep& step)
{
    {
        const auto *s = dynamic_cast<const mdl::CodeGeneration::ProcessEntityStep *>(&step);
        if (s != nullptr) {
            ui.progressBar->setValue(ui.progressBar->value() + 1);
            ui.codeGenLogsTextArea->appendPlainText(tr("Processing ") + s->entityName() + "...");
        }
    }
}

void CodeGenerationDialog::Detail::handleCodeGenerationError(Ui::CodeGenerationDialogUi& ui,
                                                             CodeGenerationError const& error)
{
    switch (error.kind) {
    case CodeGenerationError::Kind::PythonError: {
        ui.codeGenLogsTextArea->appendPlainText(tr("Python script error:"));
        ui.codeGenLogsTextArea->appendPlainText(QString::fromStdString(error.message));
        return;
    }
    case CodeGenerationError::Kind::ScriptDefinitionError: {
        ui.codeGenLogsTextArea->appendPlainText(tr("User provided an invalid Python script."));
        ui.codeGenLogsTextArea->appendPlainText(tr("Code generation module returned the following message:"));
        ui.codeGenLogsTextArea->appendPlainText(QString::fromStdString(error.message));
        ui.codeGenLogsTextArea->appendPlainText("");
        ui.codeGenLogsTextArea->appendPlainText(
            tr("For more information about how to build the generation scripts, please visit the documentation."));
        return;
    }
    }
}

QString CodeGenerationDialog::Detail::executablePath() const
{
    return QCoreApplication::applicationDirPath();
}

class HtmlPainterDelegate : public QStyledItemDelegate {
  public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        // Intercept the drawing for each item to add HTML
        auto rect = option.rect;
        auto textOffset = QPointF{20, -4};
        auto doc = QTextDocument{};
        doc.setHtml(index.data(Qt::DisplayRole).toString());
        painter->save();
        painter->translate(rect.left() + textOffset.x(), rect.top() + textOffset.y());
        doc.drawContents(painter, QRect(0, 0, rect.width(), rect.height()));
        painter->restore();

        QStyledItemDelegate::paint(painter, option, index);
    }

    [[nodiscard]] QString displayText(const QVariant& value, const QLocale& locale) const override
    {
        // This is a hack to trick QStyledItemDelegate::paint and avoiding displaying the text.
        // This is necessary because this class is drawing the text using HTML, so the QStyledItemDelegate must not
        // write anything.
        return "";
    }
};

CodeGenerationDialog::CodeGenerationDialog(ICodeGenerationDataProvider& dataProvider,
                                           std::unique_ptr<Detail> impl,
                                           QWidget *parent):
    d(std::make_unique<CodeGenerationDialog::Private>(dataProvider)),
    impl(impl == nullptr ? std::make_unique<CodeGenerationDialog::Detail>() : std::move(impl))
{
    d->ui.setupUi(this);
    d->ui.runCancelGroup->setStyleSheet("QGroupBox { border: 0; }");
    d->ui.searchGroup->setStyleSheet("QGroupBox { border: 0; }");
    d->ui.titleLabel->setStyleSheet("QLabel { font-weight: bold; }");
    d->ui.topMessageWidget->hide();
    d->ui.physicalEntitiesTree->setModel(&d->treeModel);
    d->ui.physicalEntitiesTree->setItemDelegate(new HtmlPainterDelegate{});

    d->findOnTreeModel.setTreeView(d->ui.physicalEntitiesTree);
    connect(d->ui.searchValue, &QLineEdit::textChanged, &d->findOnTreeModel, &FindOnTreeModel::findText);
    connect(d->ui.searchGoToNextBtn, &QPushButton::pressed, &d->findOnTreeModel, &FindOnTreeModel::goToNextItem);
    connect(d->ui.searchGoToPrevBtn, &QPushButton::pressed, &d->findOnTreeModel, &FindOnTreeModel::goToPrevItem);

    d->ui.statusGroup->hide();

    populateAvailableScriptsCombobox();

    connect(d->ui.findOutputDirBtn, &QAbstractButton::clicked, this, &CodeGenerationDialog::searchOutputDir);
    connect(d->ui.runCodeGenerationBtn, &QAbstractButton::clicked, this, &CodeGenerationDialog::runCodeGeneration);
    connect(d->ui.cancelBtn, &QAbstractButton::clicked, this, &CodeGenerationDialog::close);
    connect(d->ui.openOutputDir, &QAbstractButton::clicked, this, &CodeGenerationDialog::openOutputDir);

    connect(d->ui.selectAllCheckbox, &QCheckBox::stateChanged, this, [&](int newState) {
        if (newState == Qt::CheckState::PartiallyChecked) {
            d->ui.selectAllCheckbox->setCheckState(Qt::CheckState::Checked);
            return;
        }

        if (newState == Qt::CheckState::Checked) {
            d->treeModel.recursiveExec([&](QStandardItem *item) {
                item->setCheckState(Qt::CheckState::Checked);
                return CodeGenerationEntitiesTreeModel::RecursiveExec::ContinueSearch;
            });
        }

        if (newState == Qt::CheckState::Unchecked) {
            d->treeModel.recursiveExec([&](QStandardItem *item) {
                item->setCheckState(Qt::CheckState::Unchecked);
                return CodeGenerationEntitiesTreeModel::RecursiveExec::ContinueSearch;
            });
        }
    });
}

CodeGenerationDialog::~CodeGenerationDialog() = default;

void CodeGenerationDialog::runCodeGeneration()
{
    d->ui.topMessageWidget->hide();
    d->ui.progressBar->reset();
    d->ui.codeGenLogsTextArea->clear();

    if (d->ui.outputDirInput->text().isEmpty()) {
        impl->handleOutputDirEmpty(d->ui);
        return;
    }

    d->ui.progressBar->setMinimum(0);
    d->ui.progressBar->setMaximum(d->dataProvider.numberOfPhysicalEntities());
    auto iterationCallback = [&](const mdl::CodeGeneration::CodeGenerationStep& step) {
        impl->codeGenerationIterationCallback(d->ui, step);
        qApp->processEvents();
    };

    d->ui.statusGroup->show();

    // TODO [#441]: Make a CANCEL button for the code generation
    d->ui.runCodeGenerationBtn->setEnabled(false);

#if 0
    auto result = CodeGeneration::generateCodeFromScript(selectedScriptPath().toStdString(),
                                                         d->ui.outputDirInput->text().toStdString(),
                                                         d->dataProvider,
                                                         iterationCallback);
#else
    auto result = CodeGeneration::generateCodeFromjS(selectedScriptPath(),
                                                     d->ui.outputDirInput->text(),
                                                     d->dataProvider,
                                                     iterationCallback);
#endif

    d->ui.runCodeGenerationBtn->setEnabled(true);

    if (result.has_error()) {
        impl->handleCodeGenerationError(d->ui, result.error());
        return;
    }

    d->ui.codeGenLogsTextArea->appendPlainText(tr("All files processed successfully."));
    d->ui.progressBar->setValue(d->ui.progressBar->maximum());
}

void CodeGenerationDialog::populateAvailableScriptsCombobox()
{
    auto const SCRIPTS_PATH = (impl->executablePath() + "/python/codegeneration/").toStdString();
    if (!std::filesystem::exists(SCRIPTS_PATH)) {
        impl->showErrorMessage(d->ui, tr("Couldn't find any scripts for code generation."));
        return;
    }
    for (auto const& entry : std::filesystem::directory_iterator(SCRIPTS_PATH)) {
        auto const& path = entry.path();
        d->ui.availableScriptsCombobox->addItem(QString::fromStdString(path.stem().string()),
                                                QString::fromStdString(path.string()));
    }
}

void CodeGenerationDialog::searchOutputDir()
{
    auto directory = impl->getExistingDirectory(*this, d->ui.outputDirInput->text());
    if (directory.isEmpty()) {
        return;
    }
    d->ui.outputDirInput->setText(directory);
}

void CodeGenerationDialog::setOutputDir(const QString& dir)
{
    d->ui.outputDirInput->setText(dir);
}

QString CodeGenerationDialog::outputDir() const
{
    return d->ui.outputDirInput->text();
}

QString CodeGenerationDialog::selectedScriptPath() const
{
    return d->ui.availableScriptsCombobox->currentData().toString() + "/codegenerator.py";
}

void CodeGenerationDialog::openOutputDir() const
{
    QDesktopServices::openUrl(d->ui.outputDirInput->text());
}

} // namespace Codethink::lvtcgn::gui
