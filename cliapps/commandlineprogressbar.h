// commandlineprogressbar.h                                                   -*-C++-*-

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

#ifndef INCLUDED_COMMANDLINEPROGRESSBAR
#define INCLUDED_COMMANDLINEPROGRESSBAR

#include <QMutex>
#include <QObject>
#include <QString>
#include <commandlineprogressbar_export.h>
#include <iostream>
#include <stdio.h>
#include <string>

class COMMANDLINEPROGRESSBAR_EXPORT CommandLineProgressBar : public QObject {
  public:
    CommandLineProgressBar();
    ~CommandLineProgressBar() noexcept override;

    void advance(const QString& advanceMessage);
    void setupProgressBar(const QString& progressBarText, int totalSteps);

  private:
    int m_totalSteps;
    int m_currentStep;
    std::string m_progressBar;
    std::string m_progressBarText;
    std::mutex m_mutex;

    void printProgress();
    int calculatePercentage();
    void fillBar();
};

#endif // INCLUDED_COMMANDLINEPROGRESSBAR
