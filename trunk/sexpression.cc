// Copyright 2006 Google Inc. All Rights Reserved.
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
// Author: psung@google.com (Phil Sung)
//
// (See description and comments in header file.)

#include <list>

#include "sexpression.h"

// ***** SExpression::const_iterator
SExpressionConstIterator::SExpressionConstIterator(const SExpression* psexp) {
  CHECK(psexp->IsList());

  if (psexp->IsNil()) {
    done_ = true;
    next_ = NULL;
  } else {
    done_ = false;
    // Since psexp is a list and not nil, it has to be a pair
    next_ = down_cast<const SExpressionPair*>(psexp);
  }
}

const SExpression& SExpressionConstIterator::operator*() const {
  return *(next_->car());
}

const SExpression* SExpressionConstIterator::operator->() const {
  return next_->car();
}

void SExpressionConstIterator::operator++() {
  if (!done_) {
    if (next_->cdr()->IsPair()) {
      next_ = down_cast<const SExpressionPair*>(
          implicit_cast<const SExpression*>(next_->cdr()));
    } else {
      done_ = true;
      next_ = NULL;
    }
  }
}

// ***** SExpression

// Returns the printed representation of the s-exp by calling
// WriteRepr on an empty string.
string SExpression::Repr() const {
  string answer;
  WriteRepr(&answer);
  return answer;
}

SExpression* SExpression::Parse(const char* sexp) {
  StringCharacterIterator char_iter(sexp);
  return ParseSexp(&char_iter);
}

SExpression::const_iterator SExpression::Begin() const {
  return SExpression::const_iterator(this);
}

SExpression::const_iterator SExpression::End() const {
  return SExpression::const_iterator();
}

// A valid integer contains an optional +/- followed by at least one
// digit.
bool SExpression::TokenIsInteger(const char* str) {
  const char* first_char = str;

  // Check that there is no leading whitespace
  SkipWhitespace(&str);
  if (str != first_char)
    return false;

  // Check that the first character past the end of the int is
  // \0.
  return IsIntToken(str);
}

SExpression* SExpression::ParseSexp(CharacterIterator* psexp) {
  SkipWhitespace(psexp);

  switch (**psexp) {
    case '(':  return ParseList(psexp);
    case '"':  return ParseString(psexp);
    case '|':  return ParseSymbolInBars(psexp);
    case '\0': return NULL;
    default:   return ParseUnquotedToken(psexp);
  }
}

SExpression* SExpression::ParseList(CharacterIterator* psexp) {
  CHECK_EQ('(', **psexp);
  ++(*psexp);
  SkipWhitespace(psexp);

  bool is_improper_list = false;
  list<SExpression*> items;
  while (**psexp != ')') {
    // TODO(psung): Do some smarter checking for improper lists, to
    // make sure that the '.' only appears in allowed places, i.e. as
    // the second-to-last token, preceded by at least one other s-exp.

    // True whenever the next token is not a "."
    bool push_item = false;

    if (**psexp == '.') {
      ++(*psexp);
      if (ascii_isspace(**psexp)) {
        is_improper_list = true;
        ++(*psexp);
      } else {
        push_item = true;
      }
    } else {
      push_item = true;
    }

    if (push_item) {
      // Push elements onto items as we find them.
      SExpression* next_item = ParseSexp(psexp);
      if (next_item == NULL) {
        // The input ends before the list is complete; we need to bail
        // out and delete all previously created s-exps.
        while (items.size() > 0) {
          SExpression* item = items.back();
          items.pop_back();
          delete item;
        }
        return NULL;
      } else {
        items.push_back(next_item);
      }
    }

    SkipWhitespace(psexp);
  }
  ++(*psexp);

  // Build up cons cells for list iteratively, starting from tail.
  SExpression* answer;
  if (is_improper_list && items.size() >= 2) {
    answer = items.back();
    items.pop_back();
  } else {
    answer = new SExpressionNil();
  }
  while (items.size() > 0) {
    answer = new SExpressionPair(items.back(), answer);
    items.pop_back();
  }
  return answer;
}

SExpression* SExpression::ParseString(CharacterIterator* psexp) {
  string str_value;
  if (FillWithDelimitedString('"', psexp, &str_value))
    return new SExpressionString(str_value);
  else
    return false;
}

SExpression* SExpression::ParseSymbolInBars(CharacterIterator* psexp) {
  string symbol_name;
  if (FillWithDelimitedString('|', psexp, &symbol_name))
    return new SExpressionSymbol(symbol_name);
  else
    return false;
}

SExpression* SExpression::ParseUnquotedToken(CharacterIterator* psexp) {
  string token;
  bool has_escaped_char = false;

  while (!(ascii_isspace(**psexp) || **psexp == ')' || **psexp == '\0')) {
    if (**psexp == '\\') {
      has_escaped_char = true;
      ++(*psexp);
    }
    token.push_back(**psexp);
    ++(*psexp);
  }

  const char* token_cstr = token.c_str();

  CHECK_NE(*token_cstr, '\0');

  // A token turns into a integer if it can be interpreted as such AND
  // it has no escaped characters.
  if (!has_escaped_char && TokenIsInteger(token_cstr))
    return new SExpressionInteger(IntegerValue(token_cstr));
  else if (token == "nil")
    return new SExpressionNil();
  else if (TokenIsAllPeriods(token_cstr))
    return NULL;  // Token containing all periods can't be turned into
                  // a symbol
  else
    return new SExpressionSymbol(token_cstr);
}

bool SExpression::FillWithDelimitedString(char delimiter,
                                          CharacterIterator* psexp,
                                          string* str) {
  CHECK_EQ(delimiter, **psexp);
  ++(*psexp);

  while (**psexp != delimiter) {
    if (**psexp == '\0')  // Unterminated string
      return false;

    if (**psexp == '\\') {
      ++(*psexp);
      if (**psexp == '\0')  // Nothing after escape char
        return false;
    }
    str->push_back(**psexp);
    ++(*psexp);
  }

  ++(*psexp);
  return true;
}

int SExpression::IntegerValue(const char* str) {
  int value;
  int values_read = sscanf(str, "%d", &value);
  CHECK_EQ(1, values_read);
  return value;
}

// ***** SExpressionPair

void SExpressionPair::WriteRepr(string* str) const {
  str->push_back('(');
  WriteReprWithoutParens(str);
  str->push_back(')');
}

// Writes the printed representation. This function correctly writes
// lists and improper lists.
void SExpressionPair::WriteReprWithoutParens(string* str) const {
  car_->WriteRepr(str);

  // If cdr is nil, this is the end of the list.
  if (!cdr_->IsNil()) {
    if (cdr_->IsPair()) {
      // This is a list or improper list with at least one pair
      // remaining.
      str->append(" ");
      down_cast<SExpressionPair*>(cdr_)->WriteReprWithoutParens(str);
    } else {
      // This is the last pair in an improper list.
      str->append(" . ");
      cdr_->WriteRepr(str);
    }
  }
}

// ***** SExpressionSymbol

SExpressionSymbol::SExpressionSymbol(const string& name_str)
    : name_(name_str.c_str()) {
  const char* name = name_str.c_str();

  needs_quoting_ = false;

  // Symbol can't be printed literally if:
  // (1) It could be misconstrued for an integer
  // (2) It contains only periods
  // (3) It contains characters which are neither alphabetic nor
  //     acceptable punctuation (as specified by IsSymbolChar)
  if (TokenIsInteger(name) || TokenIsAllPeriods(name)) {
    needs_quoting_ = true;
  } else {
    while (*name) {
      if (!IsSymbolChar(*name))
        needs_quoting_ = true;
      ++name;
    }
  }
}

void SExpressionSymbol::WriteRepr(string* str) const {
  if (needs_quoting_) {
    const char* symbol_cstr = name_.c_str();
    str->push_back('|');
    while (*symbol_cstr) {
      if (*symbol_cstr == '|')
        str->push_back('\\');
      str->push_back(*symbol_cstr);
      ++symbol_cstr;
    }
    str->push_back('|');
  } else {
    str->append(name_);
  }
}

// We only fill in positions 0..128 but the array is static so C++
// guarantees that positions 128..255 are \0, which is acceptable.
const char SExpressionSymbol::kAsciiSymbol[256]
= "................................"
  ".x..xxx...xx.xxxxxxxxxxxxx..xxxx"
  "xxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxx"
  ".xxxxxxxxxxxxxxxxxxxxxxxxxxx.xx.";
