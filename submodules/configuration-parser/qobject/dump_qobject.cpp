/*
    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>
    SPDX-License-Identifier: MIT
*/

#include "common/dump_common.h"
#include "common/string_helpers.h"

#include "parser/meta_settings.h"

#include <filesystem>
#include <cassert>

Q_LOGGING_CATEGORY(dumpSource, "dumpSource")
Q_LOGGING_CATEGORY(dumpHeader, "dumpHeader")

namespace {

void dump_source_class(MetaClass *top, std::ofstream &file) {
  // Constructors.
  file << top->name << "::" << top->name
       << "(QObject *parent) : QObject(parent)";
  for (auto &&p : top->properties) {
    if (p->default_value.size()) {
      file << ',' << std::endl
           << "\t_" << p->name << '(' << p->default_value << ')';
    }
  }
  file << std::endl;
  file << '{' << std::endl;
  file << '}' << std::endl;
  file << std::endl;

  dump_source_get_methods(file, top);
  dump_source_set_methods(file, top, false);
}

void dump_header_class(MetaClass *top, std::ofstream &file) {
  qCDebug(dumpHeader) << "Dumping class header";

  file << "class " << top->name << " : public QObject {" << std::endl;
  file << "Q_OBJECT" << std::endl;

  // Q_PROPERTY declarations
  qCDebug(dumpHeader) << "Class has:" << top->properties.size()
                      << "properties.";
  dump_header_q_properties(file, top->properties);

  file << std::endl;
  file << "public:" << std::endl;
  file << "\t" << top->name << "(QObject *parent = 0);" << std::endl;

  dump_header_properties(file, top->properties, false);
  file << "};" << std::endl << std::endl;
}

} // end unammed namespace

namespace QObjectExport {

void dump_header(
    const MetaConfiguration &conf,
    const std::string &filename,
    const std::string &exportHeader)
{
  std::ofstream header(filename);

  header << "// clang-format off" << std::endl;

  begin_header_guards(header, filename);
  header << std::endl;

  dump_notice(header);

  header << "#include <QObject>" << std::endl;

  dump_headers(header, conf.includes);

  if (conf.includes.size()) {
    header << std::endl;
  }

  if (conf.conf_namespace.size()) {
    header << "namespace " << conf.conf_namespace << " {" << std::endl;
  }

  if (conf.includes.size()) {
    header << std::endl;
  }

  if (conf.top_level_class) {
    dump_header_class(conf.top_level_class.get(), header);
  }

  if (conf.conf_namespace.size()) {
    header << "}" << std::endl;
  }

  header << std::endl;
  header << "// clang-format on" << std::endl;
}

void dump_source(const MetaConfiguration &conf, const std::string &filename)
{
  std::filesystem::path path(filename);
  std::ofstream source(path.filename().generic_string());

  source << "// clang-format off" << std::endl;

  dump_notice(source);

  source << "#include \"" << path.stem().generic_string() << ".h\"" << std::endl;
  source << "#include <QSettings>" << std::endl;
  source << std::endl;

  if (conf.conf_namespace.size()) {
    source << "namespace " << conf.conf_namespace << " {" << std::endl
           << std::endl;
  }

  if (conf.top_level_class) {
    dump_source_class(conf.top_level_class.get(), source);
  }

  if (conf.conf_namespace.size()) {
    source << "}" << std::endl;
  }

  source << std::endl;
  source << "// clang-format on" << std::endl;
}

}
