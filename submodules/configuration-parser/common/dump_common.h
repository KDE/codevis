/*
    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>
    SPDX-License-Identifier: MIT
*/

#pragma once

#include "parser/meta_settings.h"

#include <memory>

bool pass_as_const_ref(const std::string& type);

// Transforms a metaProperty into a `Parent()->OtherParent()->Property()", callable, string.
std::string get_call_chain(std::shared_ptr<MetaProperty> property, std::string suffix);

// dump all get methods from the MetaClass.
void dump_source_get_methods(std::ofstream& f, MetaClass *currClass);

// dump all set methods from the MetaClass
void dump_source_set_methods(std::ofstream& f, MetaClass *currClass, bool useRules = true);

// dump all header properties from the MetaProperty
void dump_header_properties(std::ofstream &file, const std::vector<std::shared_ptr<MetaProperty>> &properties, bool useRules = true);

// dump all q_properties from the MetaProperty
void dump_header_q_properties(std::ofstream& f,  const std::vector<std::shared_ptr<MetaProperty>> &properties);

// dump all parameters from the metaProperty
void dump_parameter(std::ofstream& file, const std::shared_ptr<MetaProperty>& property);

// dumps "this is a generated file notice"
void dump_notice(std::ofstream& f);

// dumps the header list.
void dump_headers(std::ofstream& f, const std::vector<MetaInclude> &includes);
