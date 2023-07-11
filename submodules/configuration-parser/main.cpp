/*
    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>
    SPDX-License-Identifier: MIT
*/

#include <fstream>
#include <iostream>
#include <string>
#include <cstring>

#include "parser/meta_settings.h"
#include "parser/statemachine.h"
#include "common/string_helpers.h"
#include "kconfig/dump_kconfig.h"
#include "qobject/dump_qobject.h"
#include "qsettings/dump_qsettings.h"

#include <QtGlobal>
#include <QDebug>
#include <QFileInfo>
#include <QString>

#include <iostream>
#include <filesystem>
#include <optional>

bool hasBoolOpt(std::string optName, int argc, char *argv[]) {
    for (int i = 0; i < argc; i++) {
        if (argv[i] == optName) {
            return true;
        }
    }
    return false;
}

std::string exportHeader(int argc, char *argv[])
{
    for (int i = 0; i < argc; i++) {
        QString export_file(argv[i]);
        if (export_file.contains("--with-export-header=")) {
            return export_file
                .split("=",
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
        QString::SkipEmptyParts
#else
    Qt::SkipEmptyParts
#endif
            ).at(1).toStdString();
        }
    }
    return "";
}

void show_usage(const char appname[]) {
  std::cout << "usage" << appname << " [--kconfig | --qsettings | --qobject] [--with-export-header=header] file.conf\n";
  std::cout << "\t this will generate a header / source pair\n";
  std::cout << "\t with everything you need to start coding.\n";
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    show_usage(argv[0]);
    return 0;
  }

  std::string last_opt(argv[argc-1]);

  if (last_opt == "-h" || last_opt == "--help") {
    show_usage(argv[0]);
    return 0;
  }

  QFileInfo info(QString::fromStdString(last_opt));
  if (!info.exists()) {
    std::cout << "The specified file doesn't exist: " << last_opt << "\n";
    return 0;
  }

  const std::string file_name = info.absoluteFilePath().toStdString();
  std::ifstream file(file_name);
  if ((file.rdstate() & std::ifstream::failbit) != 0) {
    qDebug() << "could not open file.";
    return 0;
  }

  const bool exportKConfig = hasBoolOpt("--kconfig", argc, argv);
  const bool exportQObject = hasBoolOpt("--qobject", argc, argv);
  const bool exportQSettings = hasBoolOpt("--qsettings", argc, argv) || !(exportKConfig || exportQObject);
  const std::string exportHeaderFile = exportHeader(argc, argv);

  MetaConfiguration conf = parse_configuration(file);

  std::string outfile = std::filesystem::path(file_name).filename().string();

  size_t substrSize = outfile.find_last_of('.');
  std::string name_without_ext = outfile.substr(0, substrSize);

  if (exportKConfig) {
    KConfigExport::dump_header(conf, name_without_ext + ".h", exportHeaderFile);
    KConfigExport::dump_source(conf, name_without_ext + ".cpp");
  } else if (exportQObject) {
    QObjectExport::dump_header(conf, name_without_ext + ".h", exportHeaderFile);
    QObjectExport::dump_source(conf, name_without_ext + ".cpp");
  } else if (exportQSettings) {
    QSettingsExport::dump_header(conf, name_without_ext + ".h", exportHeaderFile);
    QSettingsExport::dump_source(conf, name_without_ext + ".cpp");
  }

  std::cout << "files generated " << std::filesystem::absolute(name_without_ext + ".h").string() << "\n";
  std::cout << "files generated " << std::filesystem::absolute(name_without_ext + ".cpp").string() << "\n";
}
