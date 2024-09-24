// ct_lvtqtw_bulkedit.cpp                               -*-C++-*-

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

#include "JsonEditorDialog.h"
#include "ct_lvtqtc_graphicsscene.h"
#include "ct_lvtqtc_lakosentity.h"
#include "ct_lvtshr_graphstorage.h"

#include <KMessageWidget>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <QFile>

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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

BulkEdit::BulkEdit(QWidget *parent): QDialog(parent)
{
    auto *editor = KTextEditor::Editor::instance();
    auto *doc = editor->createDocument(this);
    _view = doc->createView(nullptr);

    auto *documentation = new QTextBrowser(this);
    auto buttonBox = new QDialogButtonBox();

    doc->setHighlightingMode("json");

    auto *btnOpen = buttonBox->addButton(tr("Open"), QDialogButtonBox::ButtonRole::ActionRole);
    auto *btnSave = buttonBox->addButton(tr("Save"), QDialogButtonBox::ButtonRole::ActionRole);
    auto *btnApply = buttonBox->addButton(tr("Apply"), QDialogButtonBox::ActionRole);

    auto *splitter = new QSplitter(this);
    auto *mainLayout = new QBoxLayout(QBoxLayout::Direction::TopToBottom);
    _messageWidget = new KMessageWidget();
    _messageWidget->setVisible(false);

    QFile markdown(":/json_transform_plugin/documentation");
    markdown.open(QIODevice::ReadOnly);
    QTextStream streamReader(&markdown);
    documentation->setMarkdown(streamReader.readAll());

    splitter->addWidget(_view);
    splitter->addWidget(documentation);

    mainLayout->addWidget(splitter);
    mainLayout->addWidget(_messageWidget);
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

    connect(btnApply, &QPushButton::clicked, this, &BulkEdit::triggerJsonChange);
}

struct Filters {
    bool filterText;
    QString text;
    bool filterQualName;
    QString qualName;
    bool filterColor;
    QColor color;
    bool filterExpanded;
    bool expanded;
    bool filterEmpty;
    bool empty;
    bool filterChildren;
    bool children;
};

Filters jsonToFilters(QJsonObject& filterObj)
{
    const QList<QString> filterKeys = filterObj.keys();
    const auto [filterText, text] = [&filterKeys, &filterObj]() -> std::pair<bool, QString> {
        return filterKeys.contains("text") ? std::make_pair(true, filterObj["text"].toString())
                                           : std::make_pair(false, QString());
    }();

    const auto [filterQualName, qualName] = [&filterKeys, &filterObj]() -> std::pair<bool, QString> {
        return filterKeys.contains("qualified_name") ? std::make_pair(true, filterObj["color"].toString())
                                                     : std::make_pair(false, QString());
    }();
    const auto [filterColor, color] = [&filterKeys, &filterObj]() -> std::pair<bool, QColor> {
        return filterKeys.contains("color") ? std::make_pair(true, filterObj["color"].toString())
                                            : std::make_pair(false, QColor());
    }();

    const auto [filterEmpty, empty] = [&filterKeys, &filterObj]() -> std::pair<bool, bool> {
        return filterKeys.contains("is_empty") ? std::make_pair(true, filterObj["is_empty"].toBool())
                                               : std::make_pair(false, false);
    }();
    const auto [filterExpanded, expanded] = [&filterKeys, &filterObj]() -> std::pair<bool, bool> {
        return filterKeys.contains("is_expanded") ? std::make_pair(true, filterObj["is_expanded"].toBool())
                                                  : std::make_pair(false, false);
    }();
    const auto [filterChildren, children] = [&filterKeys, &filterObj]() -> std::pair<bool, bool> {
        return filterKeys.contains("has_unloaded_elements")
            ? std::make_pair(true, filterObj["has_unloaded_elements"].toBool())
            : std::make_pair(false, false);
    }();

    return Filters{filterText,
                   text,
                   filterQualName,
                   qualName,
                   filterColor,
                   color,
                   filterEmpty,
                   empty,
                   filterExpanded,
                   expanded,
                   filterChildren,
                   children};
}

std::vector<Codethink::lvtqtc::LakosEntity *> filterEntities(Filters f, Codethink::lvtqtc::GraphicsScene *scene)
{
    std::vector<Codethink::lvtqtc::LakosEntity *> filteredEntities;
    for (Codethink::lvtqtc::LakosEntity *e : scene->allEntities()) {
        std::cout << "Filtering entity " << e->qualifiedName() << std::endl;
        // TODO: add API to query for all children.
        // if (f.filterChildren) {
        //    if (e-)
        // }
        if (f.filterColor) {
            if (e->color() == f.color) {
                filteredEntities.push_back(e);
                continue;
            }
        }
        if (f.filterEmpty) {
            if (e->lakosEntities().empty() == f.empty) {
                filteredEntities.push_back(e);
                continue;
            }
        }

        if (f.filterExpanded) {
            if (e->isExpanded() == f.expanded) {
                filteredEntities.push_back(e);
                continue;
            }
        }

        if (f.filterQualName) {
            if (e->qualifiedName() == f.qualName.toStdString()) {
                filteredEntities.push_back(e);
                continue;
            }
        }

        // TODO: Add API for text instead of name.
        if (f.filterText) {
            if (e->name() == f.text.toStdString()) {
                filteredEntities.push_back(e);
                continue;
            } else {
                std::cout << "name differs!" << std::endl;
            }
        }
    }
    return filteredEntities;
}

void BulkEdit::triggerJsonChange()
{
    QJsonParseError error;
    auto jsonDoc = QJsonDocument::fromJson(_view->document()->text().toLocal8Bit(), &error);
    if (error.error != QJsonParseError::ParseError::NoError) {
        _messageWidget->setText(error.errorString());
        _messageWidget->setVisible(true);
        return;
    }

    if (!jsonDoc.isArray()) {
        _messageWidget->setText(tr("ERROR: Main JSON Object is not array"));
        _messageWidget->setVisible(true);
        return;
    }

    _messageWidget->setText(QString());

    QJsonArray array = jsonDoc.array();
    for (const auto& element : array) {
        QJsonObject obj = element.toObject();
        const auto allKeys = obj.keys();
        if (!allKeys.contains("filter")) {
            _messageWidget->setText(tr("ERROR: Missing required object 'filters'"));
            _messageWidget->setVisible(true);

            return;
        }
        if (!allKeys.contains("map")) {
            _messageWidget->setText(tr("ERROR: Missing required object 'map'"));
            _messageWidget->setVisible(true);
            return;
        }

        QJsonObject filterObj = obj["filter"].toObject();
        QJsonObject mapObj = obj["map"].toObject();
        Filters f = jsonToFilters(filterObj);
        const auto filteredEntities = filterEntities(f, _scene);

        for (Codethink::lvtqtc::LakosEntity *entity : filteredEntities) {
            if (mapObj.contains("text")) {
                entity->setName(mapObj["text"].toString().toStdString());
            }
            if (mapObj.contains("color")) {
                entity->setColor(QColor(mapObj["color"].toString()));
            }
            entity->update();
        }
    }
}

void BulkEdit::setScene(Codethink::lvtqtc::GraphicsScene *scene)
{
    _scene = scene;
}
