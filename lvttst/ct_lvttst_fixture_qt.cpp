// ct_lvttst_fixture_qt.cpp                                                -*-C++-*-

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

#include <ct_lvttst_fixture_qt.h>
#include <ct_lvttst_testcfgoptions.h>

void testMessageHandler(QtMsgType msgType, const QMessageLogContext& context, const QString& message)
{
    // clang-tidy cert-err33-c requires us to check the return value of printf
    auto checkRet = [](int ret) {
        assert(ret > 0);
        (void) ret;
    };

    QByteArray localMsg = message.toLocal8Bit();
    switch (msgType) {
    case QtDebugMsg:
        checkRet(fprintf(stderr, "Debug: %s\n", localMsg.constData()));
        break;
    case QtInfoMsg:
        checkRet(fprintf(stderr, "Info: %s\n", localMsg.constData()));
        break;
    case QtWarningMsg:
        checkRet(fprintf(stderr, "Warning: %s\n", localMsg.constData()));
        break;
    case QtCriticalMsg:
        checkRet(fprintf(stderr, "Critical: %s\n", localMsg.constData()));
        break;
    case QtFatalMsg:
        checkRet(fprintf(stderr, "Fatal: %s\n", localMsg.constData()));
        abort();
    }
}

QTApplicationFixture::QTApplicationFixture(): qapp(TestCfgOptions::instance().argc, TestCfgOptions::instance().argv)
{
    qInstallMessageHandler(testMessageHandler);
}

void QTApplicationFixture::processEvents(int n)
{
    while (n--) {
        QApplication::processEvents(QEventLoop::AllEvents);
    }
}

void QTApplicationFixture::interact()
{
    // Only meant to be used for debug. Creates a point of interaction with GUI
    QApplication::exec();
}
