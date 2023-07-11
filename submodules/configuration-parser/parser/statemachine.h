/*
    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>
    SPDX-License-Identifier: MIT
*/

#pragma once

#include "meta_settings.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(parser)

/* This somewhat cryptic code defines a return type
 * for a function that returns a function of the
 * same parameters as defined in the typedef.
 */
template <typename... T> struct RecursiveHelper {
  typedef std::function<RecursiveHelper(T...)> type;
  RecursiveHelper(type f) : func(f) {}
  operator type() { return func; }
  type func;
};

/* defines a return type for a function that has std::ifstream& and int&
 * parameters. */
typedef RecursiveHelper<MetaConfiguration &, std::ifstream &, int &>::type
    callback_t;

Q_DECLARE_LOGGING_CATEGORY(callbackDebug)

/* handles the beginning of the parsing, #includes, comments and classes. */
callback_t initial_state(MetaConfiguration &conf, std::ifstream &f, int &error);

/* handles the definition of new classes */
callback_t begin_class_state(MetaConfiguration &conf, std::ifstream &f,
                             int &error);

/* deals with the internals of a new class */
callback_t class_state(MetaConfiguration &conf, std::ifstream &f, int &error);

/* finishes the current class */
callback_t end_class_state(MetaConfiguration &conf, std::ifstream &f,
                           int &error);

/* handles the creation of properties for the current class */
callback_t begin_property_state(MetaConfiguration &conf, std::ifstream &f,
                                int &error);

callback_t property_state(MetaConfiguration &conf, std::ifstream &f,
                          int &error);

/* finishes the current property */
callback_t end_property_state(MetaConfiguration &conf, std::ifstream &f,
                              int &error);

#if 0
/* marks the current object (which can be a property or a class) as an array. */
callback_t begin_array_state(MetaConfiguration& conf, std::ifstream& f, int& error);

/* reads the contents of the array */
callback_t array_state(MetaConfiguration& conf, std::ifstream& f, int& error);

/* finishes the array. */
callback_t end_array_state(MetaConfiguration& conf, std::ifstream& f, int& error);
#endif

/* starts a list of possible property values. */
callback_t begin_property_set_state(MetaConfiguration &conf, std::ifstream &f,
                                    int &error);

/* handles the definition of a new set value for the property. */
callback_t property_set_value_state(MetaConfiguration &conf, std::ifstream &f,
                                    int &error);

/* finishes the handling of the property. */
callback_t end_property_set_state(MetaConfiguration &conf, std::ifstream &f,
                                  int &error);

/* reads a include string. */
callback_t state_include(MetaConfiguration &conf, std::ifstream &f, int &error);

/* reads a documentation line */
callback_t single_line_documentation_state(MetaConfiguration &conf,
                                           std::ifstream &f, int &error);

/*reads a documentation block */
callback_t multi_line_documentation_state(MetaConfiguration &conf,
                                          std::ifstream &f, int &error);

/* reads a string that we don't know what will do with it yet, maybe it's a
 * class, a property, a type.*/
callback_t multi_purpose_string_state(MetaConfiguration &conf, std::ifstream &f,
                                      int &error);

callback_t guess_documentation_state(MetaConfiguration &conf, std::ifstream &f,
                                     int &error);

/* starts to parse a configuration, and returns a MetaConfiguration object -
 * basically an AST of the doc. */
MetaConfiguration parse_configuration(std::ifstream &f);
