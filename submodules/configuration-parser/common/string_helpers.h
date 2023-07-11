/*
    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>
    SPDX-License-Identifier: MIT
*/

#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <QLoggingCategory>

/**
 * clear empty characters (spaces, line breaks)
 * from the file being read)
 *
 * @param file the file being stripped of empty characters.
 * */
void clear_empty(std::ifstream &file);

/**
 * receive a string in CamelCase and transform it
 * into camel_case
 *
 * @param s the string to be converted.
 * @return the converted string
 * */
std::string camel_case_to_underscore(const std::string &s);

/**
 * Receive a string in under_score and transform it into
 * UnderScore
 *
 * @param s the string to be converted
 * @return the converted string
 * */
std::string underscore_to_camel_case(const std::string &s);

/**
 * Capitalize the letter on the string defined by the
 * position 'pos'
 *
 * @param s the string that will have a letter capitalized
 * @param pos the index of the letter to be capitalized
 * @return the capitalized string.
 *
 * */
std::string capitalize(const std::string &s, int pos = 0);

/**
 * Deapitalize the letter on the string defined by the
 * position 'pos'
 *
 * @param s the string that will have a letter Decapitalized
 * @param pos the index of the letter to be capitalized
 * @return the capitalized string.
 *
 * */
std::string decapitalize(const std::string &s, int pos = 0);

/* read a string untill it finds a delimiter */
std::string read_untill_delimiters(std::ifstream &f,
                                   const std::vector<char> &delimiters);

/**
 * Adds support for std::string into the debug enviroment.
 */
QDebug &operator<<(QDebug &debug, const std::string &s);

/**
 * Adds support for std:vector into the debug enviroment.
 */
QDebug &operator<<(QDebug &debug, const std::vector<std::string> &vector);

/*
template<typename T>
QDebug& operator<<(QDebug& debug, const std::vector<T>& vector) {
    std::ostringstream stream;
    std::copy(begin(vector), end(vector), std::ostream_iterator<T>(stream, ",
")); stream << vector.back(); debug << stream.str();
}
*/

void begin_header_guards(std::ofstream &f, const std::string &filename);
