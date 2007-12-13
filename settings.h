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

#ifndef TOOLS_TAGS_SETTINGS_H__
#define TOOLS_TAGS_SETTINGS_H__

#include <list>
#include <ext/hash_map>

#include "strutil.h"
#include "tagsutil.h"

// Forward declaration.
class DataSource;

// A map from [language] to (definitions, callers) DataSource* pairs.
typedef hash_map<string, pair<DataSource*, DataSource*> > LanguageMap;

// A map from [corpus][language] to (definitions, callers) DataSource* pairs.
typedef hash_map<string, LanguageMap> DataSourceMap;

// A singleton class providing static access to global GTags settings.
class Settings {
 public:
  static void Load(const string& config_file);
  static void Free();
  static Settings* instance() { return instance_; }

  const string& default_corpus() const { return default_corpus_; }
  const string& default_language() const { return default_language_; }
  bool default_callers() const { return default_callers_; }
  const DataSourceMap& sources() const { return sources_; }
  const list<string>& corpuses() const { return corpuses_; }
  const list<string>& languages() const { return languages_; }

  void set_default_corpus(const string& default_corpus) {
    default_corpus_ = default_corpus;
  }
  void set_default_language(const string& default_language) {
    default_language_ = default_language;
  }
  void set_default_callers(bool default_callers) {
    default_callers_ = default_callers;
  }

  // Add a data source with the given language.
  // The pair sources contains a definitions DataSource and a possibly optional
  // callers DataSource.
  void AddDataSource(const string& corpus, const string& language,
                     pair<DataSource*, DataSource*> sources) {
    sources_[corpus][language] = sources;
  }

 private:
  Settings(const string& config_file);

  static Settings* instance_;  // Singleton instance.

  string default_corpus_;
  string default_language_;
  bool default_callers_;
  DataSourceMap sources_;
  list<string> corpuses_;
  list<string> languages_;

  DISALLOW_EVIL_CONSTRUCTORS(Settings);
};

#endif  // TOOLS_TAGS_SETTINGS_H__
