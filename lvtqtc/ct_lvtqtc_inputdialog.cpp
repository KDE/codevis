// ct_lvtqtc_inputdialog.cpp                                    -*-C++-*-

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

#include <ct_lvtqtc_inputdialog.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QShowEvent>
#include <QSpinBox>

using namespace Codethink::lvtqtc;

struct InputDialog::Private {
    std::map<QByteArray, QWidget *> entities;
    QFormLayout *mainLayout = nullptr;
    QDialogButtonBox *buttonBox = nullptr;
    QWidget *firstWidget = nullptr;
    bool finishedBuild = false;
    bool justShown = false;
    std::function<bool(void)> doExec;
};

InputDialog::InputDialog(const QString& title, QWidget *parent):
    QDialog(parent), d(std::make_unique<InputDialog::Private>())
{
    setWindowFlags(Qt::Tool | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    d->mainLayout = new QFormLayout();
    setLayout(d->mainLayout);
    setWindowTitle(title);

    d->buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(d->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(d->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    adjustSize();
    setFixedSize(size());
}

InputDialog::~InputDialog() = default;

void InputDialog::prepareFields(const std::vector<std::pair<QByteArray, QString>>& fields)
{
    for (const auto& [fieldId, labelText] : fields) {
        addTextField(fieldId, labelText);
    }
    finish();
}

void InputDialog::setTextFieldValue(const QByteArray& fieldId, const QString& value)
{
    auto *lineEdit = qobject_cast<QLineEdit *>(d->entities.at(fieldId));
    lineEdit->setText(value);
}

void InputDialog::addTextField(const QByteArray& fieldId, const QString& labelText)
{
    auto *textField = new QLineEdit();

    // HACK: Qt is always selecting the text, it doesn't matter how I
    // ask it to deselect. this is needed so the user doesn't remove
    // the parent name on a mistype.
    connect(textField, &QLineEdit::selectionChanged, this, [this, textField] {
        if (d->justShown) {
            textField->deselect();
            d->justShown = false;
        }
    });

    textField->setObjectName(fieldId);
    registerField(fieldId, labelText, textField);
}

void InputDialog::addComboBoxField(const QByteArray& fieldId,
                                   const QString& labelText,
                                   const std::vector<QString>& options)
{
    auto *combobox = new QComboBox();
    combobox->setObjectName(fieldId);
    for (const auto& v : options) {
        combobox->addItem(v);
    }
    registerField(fieldId, labelText, combobox);
}

void InputDialog::addSpinBoxField(const QByteArray& fieldId,
                                  const QString& labelText,
                                  std::pair<int, int> minMax,
                                  int value)
{
    auto *spinbox = new QSpinBox();
    spinbox->setObjectName(fieldId);
    spinbox->setMinimum(minMax.first);
    spinbox->setMaximum(minMax.second);
    spinbox->setValue(value);
    registerField(fieldId, labelText, spinbox);
}

void InputDialog::finish()
{
    assert(!d->finishedBuild && "finish() called twice");
    d->mainLayout->addWidget(d->buttonBox);
    d->finishedBuild = true;
}

void InputDialog::registerField(const QByteArray& fieldId, const QString& labelText, QWidget *widget)
{
    assert(!d->finishedBuild && "Cannot add more fields after finish() was called");

    auto *labelWidget = new QLabel(labelText);
    d->mainLayout->addWidget(labelWidget);
    d->mainLayout->addWidget(widget);
    d->entities[fieldId] = widget;
    if (!d->firstWidget) {
        d->firstWidget = widget;
    }
}

std::any InputDialog::fieldValue(const QByteArray& fieldId) const
{
    assert(d->entities.count(fieldId) != 0);

    auto className = std::string(d->entities[fieldId]->metaObject()->className());
    if (className == "QLineEdit") {
        return std::make_any<QString>(qobject_cast<QLineEdit *>(d->entities[fieldId])->text());
    }
    if (className == "QComboBox") {
        return std::make_any<QString>(qobject_cast<QComboBox *>(d->entities[fieldId])->currentText());
    }
    if (className == "QSpinBox") {
        return std::make_any<int>(qobject_cast<QSpinBox *>(d->entities[fieldId])->value());
    }

    assert(false && "Unkwnown data type");
    return "";
}

void InputDialog::showEvent(QShowEvent *ev)
{
    QDialog::showEvent(ev);
    d->justShown = true;
    if (d->firstWidget) {
        d->firstWidget->setFocus(Qt::MouseFocusReason);
        if (auto *lineEdit = qobject_cast<QLineEdit *>(d->firstWidget)) {
            lineEdit->deselect();
        }
    }
}

void InputDialog::overrideExec(std::function<bool(void)> exec)
{
    d->doExec = std::move(exec);
}

int InputDialog::exec()
{
    if (d->doExec) {
        return d->doExec();
    }
    return QDialog::exec();
}
