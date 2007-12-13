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
// The internal config file format uses data-centers to generat target addresses
// for RPCs.
//
// The socket config file format uses language-host and language-port pairs as
// target addresses for RPCs.  This means that they must be read into two
// different hash maps.  The alternative format is a language-host-port tuple,
// but this would require considerable rework to the SExpression Read*
// functions.  Consequently, we chose the format of two pairs was for the socket
// config file.

#include "settings.h"

#include "socket_tags_service.h"

#include "datasource.h"
#include "sexpression.h"
#include "sexpression_util-inl.h"
#include "tagsoptionparser.h"

DEFINE_STRING(default_corpus,
              "corpus1",
              "default query corpus.");
DEFINE_STRING(default_language, "c++", "default query language.");
DEFINE_BOOL(default_callers, false, "default query callgraph.");

Settings* Settings::instance_;

void Settings::Load(const string& config_file) {
  Settings::instance_ = new Settings(config_file);
}

void Settings::Free() {
  delete Settings::instance_;
  Settings::instance_ = NULL;
}

Settings::Settings(const string& config_file) {
  default_corpus_ = GET_FLAG(default_corpus);
  default_language_ = GET_FLAG(default_language);
  default_callers_ = GET_FLAG(default_callers);

  FileReader<SExpression> file_reader(config_file);
  hash_map<string, bool> has_callgraph;
  hash_map<string, string> language_hostnames;
  hash_map<string, string> callgraph_hostnames;
  hash_map<string, int> language_ports;
  hash_map<string, int> callgraph_ports;

  while (!file_reader.IsDone()) {
    const SExpression* s = file_reader.GetNext();
    if (s) {
      // All known settings are lists for now.
      if (!s->IsList()) {
        LOG(WARNING) << "Skipping: " << s->Repr();
        delete s;
        continue;
      }

      // All known settings start with a symbol followed by a list of values.
      SExpression::const_iterator iter = s->Begin();
      if (iter != s->End() && iter->IsSymbol()) {
        const string& setting =
            down_cast<const SExpressionSymbol*>(&(*iter))->name();
        ++iter;
        if (setting == "gtags-corpuses") {
          ReadList(iter, s->End(), &corpuses_);
        } else if (setting == "gtags-languages") {
          ReadList(iter, s->End(), &languages_);
        } else if (setting == "gtags-language-has-callgraph") {
          ReadPairList(iter, s->End(), &has_callgraph);
        } else if (setting == "gtags-language-hostnames") {
          ReadPairList(iter, s->End(), &language_hostnames);
        } else if (setting == "gtags-callgraph-hostnames") {
          ReadPairList(iter, s->End(), &callgraph_hostnames);
        } else if (setting == "gtags-language-ports") {
          ReadPairList(iter, s->End(), &language_ports);
        } else if (setting == "gtags-callgraph-ports") {
          ReadPairList(iter, s->End(), &callgraph_ports);
        }
      } else {
        LOG(WARNING) << "Skipping: " << iter->Repr();
      }
      delete s;
    }
  }

  // Add sources based on settings.
  for (list<string>::const_iterator corpus = corpuses_.begin();
       corpus != corpuses_.end(); ++corpus) {
    for (list<string>::const_iterator lang = languages_.begin();
         lang != languages_.end(); ++lang) {

      // create a data source for this language.
      pair<DataSource*, DataSource*> source_pair;
      RemoteDataSource* definition = new RemoteDataSource();
      LOG(INFO) << "Data source for (" << *lang << ", definition) created"
          ;

      // We don't have callgraph support for all the languages.
      // Create data source for only the ones with callgraph.
      RemoteDataSource* callgraph;
      hash_map<string, bool>::iterator cg_iter = has_callgraph.find(*lang);
      if (cg_iter != has_callgraph.end() && cg_iter->second) {
        callgraph = new RemoteDataSource();
        LOG(INFO) << "Data source for (" << *corpus << ", " << *lang
                  << ", callers) created.";
      } else {
        callgraph = NULL;
        LOG(INFO) << "Data source for (" << *corpus << ", " << *lang
                  << ", callers) skipped.";
      }

      hash_map<string, string>::iterator lang_hostname_iter =
          language_hostnames.find(*lang);
      hash_map<string, int>::iterator lang_port_iter =
          language_ports.find(*lang);
      if (lang_hostname_iter != language_hostnames.end()
          && lang_port_iter != language_ports.end()) {
        const string LANGUAGE_ADDRESS = lang_hostname_iter->second;
        const int LANGUAGE_PORT = lang_port_iter->second;
        definition->AddSource(
            new gtags::SocketTagsServiceUser(
                LANGUAGE_ADDRESS, LANGUAGE_PORT));
        LOG(INFO) << "Added source: " << LANGUAGE_ADDRESS
                  << ':' << LANGUAGE_PORT;
      }

      if (callgraph) {
        hash_map<string, string>::iterator cg_hostname_iter =
            callgraph_hostnames.find(*lang);
        hash_map<string, int>::iterator cg_port_iter =
            callgraph_ports.find(*lang);
        if (cg_hostname_iter != callgraph_hostnames.end()
            && cg_port_iter != callgraph_ports.end()) {
          const string CALLGRAPH_ADDRESS = cg_hostname_iter->second;
          const int CALLGRAPH_PORT = cg_port_iter->second;
          callgraph->AddSource(
              new gtags::SocketTagsServiceUser(
                  CALLGRAPH_ADDRESS, CALLGRAPH_PORT));
          LOG(INFO) << "Added source: " << CALLGRAPH_ADDRESS
                    << ':' << CALLGRAPH_PORT;
        }
      }

      source_pair.first = definition;
      source_pair.second = callgraph;
      sources_[*corpus][*lang] = source_pair;
    }
  }
}
