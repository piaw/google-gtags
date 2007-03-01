// Copyright 2007 Google Inc. All Rights Reserved.
// Author: stephenchen@google.com (Stephen Chen)

#ifndef TOOLS_TAGS_REGEXP_H__
#define TOOLS_TAGS_REGEXP_H__

#include <regex.h>
#include "tagsutil.h"

class RegExp {

 private:
  regex_t reg_;
  bool error_;

 public:
  RegExp(const string & re) : error_(false) {
    if (regcomp(&reg_, re.c_str(), REG_EXTENDED)) {
      LOG(WARNING) << "Corrupted regular expression: " << re << "\n";
      error_ = true;
    }
  }

  ~RegExp() {
    regfree(&reg_);
  }

  bool error() {
    return error_;
  }

  // In PartialMatch and FullMatch:
  // nsub is the quantity of sub-expressions (parenthesis) in the regular
  // expression.
  // pmat is the structure that regexec uses to return the position of the match
  // regexec returns 0 if there is a match and != 0 if not
  // pmat[0].rm_eo and pmat[0].rm_so are the end and the beginning of the
  // regular expression match

  bool PartialMatch(const string & str) {
    // If any substring of str matches, return true
    int nsub = reg_.re_nsub+1;
    regmatch_t *pmat;
    pmat = (regmatch_t *) malloc(sizeof(regmatch_t)*nsub);
    bool match = (regexec(&reg_, str.c_str(), nsub, pmat, 0) == 0);
    free(pmat);
    return match;
  }

  bool FullMatch(const string & str) {
    // If str matches, return true
    int nsub = reg_.re_nsub+1;
    regmatch_t *pmat;
    pmat = (regmatch_t *) malloc(sizeof(regmatch_t)*nsub);
    // It matches
    bool match = (regexec(&reg_, str.c_str(), nsub, pmat, 0) == 0);
    bool ret = false;
    // From the begginning to the end of str
    if (match) ret = ((pmat[0].rm_eo - pmat[0].rm_so) == str.size());

    free(pmat);
    return ret;
  }
};

#endif  // TOOLS_TAGS_REGEXP_H__
