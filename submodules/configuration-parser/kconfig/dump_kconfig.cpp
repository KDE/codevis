/*
    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>
    SPDX-License-Identifier: MIT
*/

#include "common/dump_common.h"
#include "common/string_helpers.h"

#include <QLoggingCategory>

#include <filesystem>

Q_LOGGING_CATEGORY(dumpKConfigSource, "dumpSource")
Q_LOGGING_CATEGORY(dumpKConfigHeader, "dumpHeader")

namespace {

void dump_source_class_settings_set_values(MetaClass *top,
                                           std::ofstream &file, const std::string& parentGroup) {
  std::string mainGroup = parentGroup;

  // KConfig API uses pointers / values =[
  std::string methodAcessor = parentGroup == "internal_config" ? "->" : ".";

  if (top->parent) {
      mainGroup = decapitalize(top->name, 0) + "Group";
    file << "\t" << "KConfigGroup " << mainGroup << " = "
        << parentGroup << methodAcessor << "group(\"" << top->name << "\");" << std::endl;
  } else {
      mainGroup = "generalGroup";
      file << "KConfigGroup generalGroup(internal_config, \"General\");";
  }

  for (auto &&s : top->subclasses) {
    file << std::endl;
    dump_source_class_settings_set_values(s.get(), file, mainGroup);
  }

  for (auto &&p : top->properties) {
    file << "\tif (" << get_call_chain(p, "") << " == " << get_call_chain(p, "Default") << "){" << std::endl;
    file << "\t\t" << mainGroup << ".deleteEntry(\""
        << camel_case_to_underscore(p->name) << "\");" << std::endl;

    file << "\t} else { " << std::endl;
    file << "\t\t" << mainGroup << ".writeEntry(\"" << camel_case_to_underscore(p->name)
         << "\",";

    if (p->is_enum) {
      file << "(int) ";
    }
    file << get_call_chain(p, "") << ");" << std::endl;
    file << "\t}" << std::endl;
  }
}

void dump_source_class_settings_get_values(MetaClass *top,
                                           std::ofstream &file,
                                           const std::string& parentGroup) {
  std::string mainGroup = parentGroup;

  // KConfig API uses pointers / values =[
  std::string methodAcessor = parentGroup == "internal_config" ? "->" : ".";

  if (top->parent) {
    mainGroup = decapitalize(top->name, 0) + "Group";
    file << "\tKConfigGroup " << mainGroup << " = "
        << parentGroup << methodAcessor << "group(\"" << top->name << "\");" << std::endl;
  } else {
      mainGroup = "generalGroup";
      file << "KConfigGroup generalGroup(internal_config, \"General\");";
  }

  for (auto &&s : top->subclasses) {
    file << std::endl;
    dump_source_class_settings_get_values(s.get(), file, mainGroup);
  }

  for (auto &&p : top->properties) {
    std::string callchain;
    auto tmp = p->parent;
    if (tmp && tmp->parent) {
      while (tmp->parent) {
        std::string s = decapitalize(tmp->name, 0) + "()->";
        callchain.insert(0, s);
        tmp = tmp->parent;
      }
    }
    file << "\t" << callchain;
    file << "set" << capitalize(p->name, 0) << "(";

    if (p->is_enum)
      file << "(" << p->type << ")";

    file << mainGroup << ".readEntry<" << p->type << ">(\"" << camel_case_to_underscore(p->name) << "\"";

    if (p->default_value.length() > 0) {
        file << ", " << p->default_value;
    }

    file << "));" << std::endl;
  }
}

bool class_or_subclass_have_properties(MetaClass *top) {
  if (!top)
    return false;
  if (top->properties.size() != 0) {
    return true;
  }

  for (auto child : top->subclasses) {
    if (class_or_subclass_have_properties(child.get())) {
      return true;
    }
  }
  return false;
}

void dump_source_class(MetaClass *top, std::ofstream &file) {
  for (auto &&child : top->subclasses) {
    dump_source_class(child.get(), file);
  }

  // Constructors.
  file << top->name << "::" << top->name
       << "(QObject *parent) : QObject(parent)";
  for (auto &&c : top->subclasses) {
    file << ',' << std::endl
         << "\t_" << decapitalize(c->name, 0) << "(new " << c->name
         << "(this))";
  }
  for (auto &&p : top->properties) {
    if (p->default_value.size()) {
      file << ',' << std::endl
           << "\t_" << p->name << '(' << p->default_value << ')';
    }
  }
  file << std::endl;
  file << '{' << std::endl;
  if (!top->parent) {
    file << "\tload();\n";
  }
  file << '}' << std::endl;
  file << std::endl;

  // get - methods.
  dump_source_get_methods(file, top);
  for (auto &&c : top->subclasses) {
    file << c->name << "* " << top->name << "::" << decapitalize(c->name)
         << "() const" << std::endl;
    file << '{' << std::endl;
    file << "\treturn _" << decapitalize(c->name) << ';' << std::endl;
    file << '}' << std::endl;
    file << std::endl;
  }

  // set-methods
  dump_source_set_methods(file, top);

  // rule-methods {
  for (auto &&p : top->properties) {
    file << "void " << top->name << "::set" << capitalize(p->name, 0)
         << "Rule(std::function<bool(" << p->type << ")> rule)" << std::endl;
    file << '{' << std::endl;
    file << '\t' << p->name << "Rule = rule;" << std::endl;
    file << '}' << std::endl;
    file << std::endl;
  }

  // default values
  for (auto &&p : top->properties) {
      if (p->default_value.size()) {
          file << p->type << " " << top->name << "::" << p->name << "Default() const" << std::endl;
          file << '{' << std::endl;
          file << '\t' << "return " << p->default_value << ';' << std::endl;
          file << '}';
          file << std::endl;
    }
  }

  // load defaults
  file << "void " << top->name << "::loadDefaults()" << std::endl;
  file << "{" << std::endl;
  for (auto &&c : top->subclasses) {
    file << "\t_" << decapitalize(c->name, 0) << "->loadDefaults();" << std::endl;
  }

  for (auto &&p : top->properties) {
    if (p->default_value.size()) {
        file << "\tset" << capitalize(p->name, 0) << '(' << p->default_value << ");" << std::endl;
    }
  }
  file << "}" << std::endl;

  // main preferences class
  if (!top->parent) {
      // The Sync method.
    file << "void " << top->name << "::sync()" << std::endl;
    file << '{' << std::endl;
    if (class_or_subclass_have_properties(top)) {
      file << "\tKSharedConfigPtr internal_config = KSharedConfig::openConfig();" << std::endl;
      dump_source_class_settings_set_values(top, file, "internal_config");
    }
    file << '}' << std::endl;
    file << std::endl;

    // The "load" method.
    file << "void " << top->name << "::load()" << std::endl;
    file << '{' << std::endl;
    if (class_or_subclass_have_properties(top)) {
      file << "\tKSharedConfigPtr internal_config = KSharedConfig::openConfig();" << std::endl;
      dump_source_class_settings_get_values(top, file, "internal_config");
    }
    file << '}' << std::endl;
    file << std::endl;
    file << top->name << "* " << top->name << "::self()" << std::endl;
    file << "{" << std::endl;
    file << "\tstatic " << top->name << " s;" << std::endl;
    file << "\treturn &s;" << std::endl;
    file << "}" << std::endl;
  }
}

void dump_header_subclasses(
    std::ofstream &file,
    const std::vector<std::shared_ptr<MetaClass>> &subclasses,
    bool has_private) {
  if (!subclasses.size())
    return;

  if (!has_private) {
    file << std::endl << "private:" << std::endl;
  }

  for (auto &&child : subclasses) {
    file << "\t" << child->name << " *_" << decapitalize(child->name, 0) << ";"
         << std::endl;
  }
}

void dump_header_class(
    MetaClass *top,
    std::ofstream &file,
    const std::string &exportExpression)
{

  qCDebug(dumpKConfigHeader) << "Dumping class header";
  for (auto &&child : top->subclasses) {
    dump_header_class(child.get(), file, exportExpression);
  }

  file << "class ";

  if (exportExpression.size() != 0) {
    file << exportExpression << " ";
  }

  file << top->name << " : public QObject {" << std::endl;
  file << "\tQ_OBJECT" << std::endl;

  // Q_PROPERTY declarations
  qCDebug(dumpKConfigHeader) << "Class has:" << top->properties.size()
                      << "properties.";
  dump_header_q_properties(file, top->properties);

  for (auto &&child : top->subclasses) {
    file << "Q_PROPERTY(QObject* " << camel_case_to_underscore(child->name)
         << " MEMBER _" << decapitalize(child->name, 0) << " CONSTANT)"
         << std::endl;
  }

  file << std::endl;
  file << "public:" << std::endl;
  if (top->parent) {
    file << "\t" << top->name << "(QObject *parent = 0);" << std::endl;
  } else {
    file << "\tvoid sync();" << std::endl;
    file << "\tvoid load();" << std::endl;
    file << "\tstatic " << top->name << "* self();" << std::endl;
  }

  file << "\t void loadDefaults();" << std::endl;

  for (auto &&child : top->subclasses) {
    file << "\t" << child->name << " *" << decapitalize(child->name, 0)
         << "() const;" << std::endl;
  }

  dump_header_properties(file, top->properties);
  dump_header_subclasses(file, top->subclasses, !top->properties.empty());

  if (!top->parent) {
    if (!top->properties.empty() || top->subclasses.empty()) {
      file << std::endl << "private:" << std::endl;
    }
    file << "\t" << top->name << "(QObject *parent = 0);" << std::endl;
  }
  file << "};" << std::endl << std::endl;
}

} // end unammed namespace

namespace KConfigExport {

void dump_header(
    const MetaConfiguration &conf,
    const std::string &filename,
    const std::string &exportHeader)
{
  qCDebug(dumpKConfigSource) << "Starting to dump the source file into" << filename;

  std::ofstream header(filename);
  // disable clang format for this, it's an automated file.

  header << "// clang-format off" << std::endl;

  begin_header_guards(header, filename);
  header << std::endl;

  dump_notice(header);

  std::string export_name;
  if (exportHeader.size() != 0) {
    header << "#include <" << exportHeader << "_export.h>" << std::endl;
    export_name = capitalize(exportHeader, -1) + "_EXPORT";
  }

  header << "#include <functional>" << std::endl;
  header << "#include <QObject>" << std::endl;

  header << std::endl;
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
    dump_header_class(conf.top_level_class.get(), header, export_name);
  }

  if (conf.conf_namespace.size()) {
    header << "}" << std::endl;
  }

  header << std::endl;
  header << "// clang-format on" << std::endl;
}

void dump_source(const MetaConfiguration &conf, const std::string &filename)
{
  std::ofstream source(filename);
  std::filesystem::path path(filename);

  source << "// clang-format off" << std::endl;

  dump_notice(source);

  source << "#include \"" << path.stem().generic_string() << ".h\""
         << std::endl;
  source << "#include <KConfig>" << std::endl;
  source << "#include <KConfigGroup>" << std::endl;
  source << "#include <KSharedConfig>" << std::endl;

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
