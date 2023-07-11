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

#include <aboutdialog.h>
#include <ui_aboutdialog.h>

#include <QFile>

#include <version.h>

#include <QDebug>

AboutDialog::AboutDialog(): ui(std::make_unique<Ui::AboutDialog>())
{
    ui->setupUi(this);

    QFile about(":/md/about");
    about.open(QIODevice::ReadOnly);

    QString aboutText = QString(about.readAll())
                            .arg(QString::fromStdString(VersionInformation::version),
                                 QString::fromStdString(VersionInformation::build_date));

    // Qt on Appimage is 5.13 aparently.
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    ui->about->setText(aboutText);
#else
    ui->about->setMarkdown(aboutText);
#endif
    about.close();

    QString authors(R"(
## Maintainers

* Tarcisio Fischer
* Tomaz Canabrava

## Developers, in Alphabetical List

    )");

    const auto gitAuthors = QString::fromStdString(VersionInformation::authors);
    const auto authorsList = gitAuthors.split("\n");

    for (const auto& author : authorsList) {
        authors += QString("\n* ") + author;
    }

    authors += "\n";
// Qt on Appimage is 5.13 aparently.
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    ui->authors->setText(authors);
#else
    ui->authors->setMarkdown(authors);
#endif

    QFile libraries(":/md/libraries");
    libraries.open(QIODevice::ReadOnly);
    auto librariesText = QString(libraries.readAll());

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    ui->thirdParty->setText(librariesText);
#else
    ui->thirdParty->setMarkdown(librariesText);
#endif
}

AboutDialog::~AboutDialog() = default;
