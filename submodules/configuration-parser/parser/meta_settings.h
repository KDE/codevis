/*
    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>
    SPDX-License-Identifier: MIT
*/

#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct MetaClass;
struct MetaProperty;

/*a struct to contain the includes*/
struct MetaInclude {
	std::string name;

    // global includes are with <> and non global with ""
	bool is_global;
};

/* struct that represents a variable in the configuration file. */
struct MetaProperty {
  typedef std::shared_ptr<MetaProperty> Ptr;
  std::shared_ptr<MetaClass> parent = nullptr;
  std::string name;
  std::string default_value;
  std::string type;
  std::map<std::string, std::string> setters;
  bool is_enum;

  std::string get_call_chain();
};

/* struct that represents a {} entity in the configuration file. */
struct MetaClass {
  typedef std::shared_ptr<MetaClass> Ptr;
  std::vector<MetaProperty::Ptr> properties;
  std::vector<Ptr> subclasses;
  std::shared_ptr<MetaClass> parent = nullptr;
  std::string name;
};

/* struct that represents the whole configuration file. */
struct MetaConfiguration {
  std::vector<MetaInclude> includes;
  std::shared_ptr<MetaClass> top_level_class = nullptr;
  std::string conf_namespace;
};
