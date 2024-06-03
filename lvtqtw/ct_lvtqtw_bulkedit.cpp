// ct_lvtqtw_buildedit.cpp                               -*-C++-*-

/*
 * // Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
 * // SPDX-License-Identifier: Apache-2.0
 * //
 * // Licensed under the Apache License, Version 2.0 (the "License");
 * // you may not use this file except in compliance with the License.
 * // You may obtain a copy of the License at
 * //
 * //     http://www.apache.org/licenses/LICENSE-2.0
 * //
 * // Unless required by applicable law or agreed to in writing, software
 * // distributed under the License is distributed on an "AS IS" BASIS,
 * // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * // See the License for the specific language governing permissions and
 * // limitations under the License.
 */

#include <ct_lvtqtw_bulkedit.h>

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <KMessageWidget>
#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QPushButton>
#include <QSplitter>
#include <QTextBrowser>
#include <QTextStream>
#include <QToolButton>

namespace Codethink::lvtqtw {

BulkEdit::BulkEdit(QWidget *parent): QDialog(parent)
{
    auto *editor = KTextEditor::Editor::instance();
    auto *doc = editor->createDocument(this);
    auto *view = doc->createView(nullptr);

    auto *documentation = new QTextBrowser(this);
    auto buttonBox = new QDialogButtonBox();

    doc->setHighlightingMode("json");

    auto *btnOpen = buttonBox->addButton(tr("Open"), QDialogButtonBox::ButtonRole::ActionRole);
    auto *btnSave = buttonBox->addButton(tr("Save"), QDialogButtonBox::ButtonRole::ActionRole);
    auto *btnApply = buttonBox->addButton(tr("Apply"), QDialogButtonBox::ApplyRole);

    auto *splitter = new QSplitter(this);
    auto *mainLayout = new QBoxLayout(QBoxLayout::Direction::TopToBottom);
    auto *messageWidget = new KMessageWidget();
    messageWidget->setVisible(false);

    QFile markdown(":/md/json_scene_elements");
    markdown.open(QIODevice::ReadOnly);
    QTextStream streamReader(&markdown);
    documentation->setMarkdown(streamReader.readAll());

    splitter->addWidget(view);
    splitter->addWidget(documentation);

    mainLayout->addWidget(splitter);
    mainLayout->addWidget(messageWidget);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    connect(btnOpen, &QPushButton::clicked, this, [this, doc] {
        const auto jsonFile = QFileDialog::getOpenFileName(this);
        doc->openUrl(QUrl::fromLocalFile(jsonFile));
    });

    connect(btnSave, &QPushButton::clicked, this, [this, doc] {
        const auto jsonFile = QFileDialog::getSaveFileName(this);
        if (jsonFile.isEmpty()) {
            return;
        }

        doc->saveAs(QUrl::fromLocalFile(jsonFile));
    });

    connect(btnApply, &QPushButton::clicked, this, [this, doc] {
        Q_EMIT sendBulkJson(doc->text());
    });
}

} // namespace Codethink::lvtqtw
