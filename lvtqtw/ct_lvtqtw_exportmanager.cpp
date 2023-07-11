// ct_lvtqtw_exportmanager.cpp                                         -*-C++-*-

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

#include <ct_lvtqtw_exportmanager.h>

#include <ct_lvtqtc_graphicsscene.h>
#include <ct_lvtqtc_graphicsview.h>

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QPainter>
#include <QSize>
#include <QSvgGenerator>
#include <Qt> // Qt::White

#include <cassert>
#include <limits>
#include <string>

namespace Codethink::lvtqtw {

struct ExportManager::Private {
    lvtqtc::GraphicsView *view;
    lvtqtc::GraphicsScene *scene;

    explicit Private(lvtqtc::GraphicsView *view): view(view)
    {
        if (view) {
            scene = qobject_cast<lvtqtc::GraphicsScene *>(view->scene());
        }
    }
};

// --------------------------------------------
// class ExportManager
// --------------------------------------------

ExportManager::ExportManager(lvtqtc::GraphicsView *view): d(std::make_unique<ExportManager::Private>(view))
{
}

ExportManager::~ExportManager() = default;

namespace {
QString getFilePath(const QString& caption, const QString& dir, const QString& filter)
{
    return QFileDialog::getSaveFileName(QApplication::activeWindow(), caption, dir, filter);
}
} // namespace

cpp::result<void, ExportError> ExportManager::exportSvg(const QString& path) const
{
    assert(d->view && d->scene);

    QString filePath = !path.isEmpty()
        ? path
        : getFilePath(QObject::tr("Save image as"), QObject::tr("diagram.svg"), QObject::tr("SVG file (*.svg)"));
    // not an error, user cancelled.
    if (filePath.isEmpty()) {
        return {};
    }

    QSvgGenerator svgGen;

    const QSize rSize = d->scene->sceneRect().size().toSize();
    svgGen.setFileName(filePath);
    svgGen.setSize(rSize);
    svgGen.setViewBox(QRect(0, 0, rSize.width(), rSize.height()));

    QPainter painter(&svgGen);
    d->scene->render(&painter);

    return {};
}

} // end namespace Codethink::lvtqtw
