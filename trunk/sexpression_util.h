// Copyright 2007 Google Inc. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// Author: stephenchen@google.com (Stephen Chen)
//
// Helper functions for processing S-Expressions.

#ifndef TOOLS_TAGS_SEXPRESSION_UTIL_H__
#define TOOLS_TAGS_SEXPRESSION_UTIL_H__

#include "strutil.h"

// Forward declaration.
class SExpression;

// Given an S-Expression in the form of ((key1 value1) (key2 value2) ...)
// and a key, return the SExpression value associated with the key.
const SExpression* SExpressionAssocGet(const SExpression* sexpr,
                                       const string& key);

// Given an S-Expression in the form of ((key1 value1) (key2 value2) ...),
// a key and a value, find the key/value pair specified by key and substitute
// the value and return the SExpression in string form.
string SExpressionAssocReplace(const SExpression* sexpr, const string& key,
                               const string& value);


#endif  // TOOLS_TAGS_SEXPRESSION_UTIL_H__
