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
// Author: psung@google.com (Phil Sung), piaw@google.com (Piaw Na)
//
// (See comments and description in tagsrequesthandler.h.)

#include "tagsrequesthandler.h"

#include <list>
#include <map>
#include <set>
#include <string>

#include "tagstable.h"
#include "sexpression.h"
#include "tagsoptionparser.h"
#include "tagsutil.h"
#include "queryprofile.h"

using gtags::MutexLock;

// If test mode flag is on, the server will answer 'nil' instead of
// 't' to the '/' or 'ping' commands, so the clients can know that the
// server is in testing mode.
DEFINE_BOOL(test_mode, false, "Enable test mode");

SingleTableTagsRequestHandler::SingleTableTagsRequestHandler
    (string tags_file, bool enable_fileindex, bool enable_gunzip,
     string corpus_root) {
  tags_table_ = new TagsTable(enable_fileindex);
  CHECK(tags_table_->ReloadTagFile(tags_file, enable_gunzip));

  opcode_handler_ = new OpcodeProtocolRequestHandler(enable_fileindex,
                                                     enable_gunzip,
                                                     corpus_root);
  sexp_handler_ = new SexpProtocolRequestHandler(enable_fileindex,
                                                 enable_gunzip,
                                                 corpus_root);
}

SingleTableTagsRequestHandler::~SingleTableTagsRequestHandler() {
  delete tags_table_;
  delete opcode_handler_;
  delete sexp_handler_;
}

string
SingleTableTagsRequestHandler::Execute(const char* command,
                                       clock_t* pclock_before_preparing_results,
                                       struct query_profile* log) {
  // We work by dispatching on protocol, which can be determined by
  // looking at the first character.
  ProtocolRequestHandler* handler
    = (*command == '(') ? sexp_handler_ : opcode_handler_;

  CHECK(pclock_before_preparing_results != NULL);
  CHECK(log != NULL);

  return handler->Execute(command,
                          tags_table_,
                          pclock_before_preparing_results,
                          log);
}

string ProtocolRequestHandler::StripCorpusRoot(const string& path) {
  if (corpus_root_ == "")
    return path;
  string pattern = "/" + corpus_root_ + "/";
  int pos = path.find(pattern);
  if (pos != string::npos)
    return path.substr(pos + pattern.size());
  else
    return path;
}

string OpcodeProtocolRequestHandler::Execute(
    const char* command,
    TagsTable* tags_table,
    clock_t* pclock_before_preparing_results,
    struct query_profile* log) {
  string output;

  // Extract leading comment, if any
  string comment;
  if (*command == '#') {
    command++;
    while (*command != '#') {
      comment += *command;
      command++;
    }
    command++;
  }

  log->client = comment;

  // Get everything after the opcode
  const char* tag = command + 1;

  log->tag = string(tag);
  log->command = *command;
  log->current_file = "";
  log->client_message = "";

  list<const TagsTable::TagsResult*>* tag_matches = NULL;
  set<string>* file_matches = NULL;

  bool search_callers = tags_table->SearchCallersByDefault();

  // Set command based on opcode and put the tag in the right place
  switch (*command) {
    case PING:  // Ping, print 't if not in testing mode
      *pclock_before_preparing_results = clock();
      output.append(GET_FLAG(test_mode) ? "nil" : "t");
      break;
    case RELOAD_TAGS_FILE:  // Reload tags table, print 't if success
      *pclock_before_preparing_results = clock();
      output.append(
          tags_table->ReloadTagFile(tag, enable_gunzip_) ? "t" : "nil");
      break;
    case LOAD_UPDATE_FILE:
      *pclock_before_preparing_results = clock();
      output.append(
          tags_table->UpdateTagFile(tag, enable_gunzip_) ? "t" : "nil");
      break;
    case FIND_FILE:  // Find file
      file_matches = tags_table->FindFile(tag);
      *pclock_before_preparing_results = clock();
      PrintFileResults(file_matches, &output);
      break;
    case LOOKUP_TAGS_IN_FILE:  // All tags in file
      *pclock_before_preparing_results = clock();
      if (enable_fileindex_) {
        string file = StripCorpusRoot(tag);
        tag_matches = tags_table->FindTagsByFile(file, search_callers);
        PrintTagsResults(tag_matches, &output);
      } else {
        output.append("nil");
      }
      break;
    case LOOKUP_TAG_PREFIX_REGEXP:  // Prefix regexp
      tag_matches = tags_table->FindRegexpTags(tag, "", search_callers, NULL);
      *pclock_before_preparing_results = clock();
      PrintTagsResults(tag_matches, &output);
      break;
    case LOOKUP_TAG_SNIPPET_REGEXP:  // Snippet regexp
      tag_matches =
          tags_table->FindSnippetMatches(tag, "", search_callers, NULL);
      *pclock_before_preparing_results = clock();
      PrintTagsResults(tag_matches, &output);
      break;
    case LOOKUP_TAG_EXACT:  // Exact match
      tag_matches = tags_table->FindTags(tag, "", search_callers, NULL);
      *pclock_before_preparing_results = clock();
      PrintTagsResults(tag_matches, &output);
      break;
    default:
      *pclock_before_preparing_results = clock();
      output.append("nil");
  }

  delete tag_matches;
  delete file_matches;

  return output;
}

void OpcodeProtocolRequestHandler::PrintTagsResults(
    list<const TagsTable::TagsResult*>* matches,
    string* output) {
  output->push_back('(');
  // Output format:
  // ((tag . (snippet filename filesize line offset)) ...)
  for (list<const TagsTable::TagsResult*>::const_iterator i = matches->begin();
       i != matches->end();
       ++i) {
    output->append("(\"");
    output->append((*i)->tag);
    output->append("\" . (\"");
    output->append(EscapeQuotes((*i)->linerep));
    output->append("\" \"");
    output->append((*i)->filename->Str());
    output->append("\" 0 ");  // filesize field is obselete; fill in 0
    output->append(FastItoa((*i)->lineno));
    output->push_back(' ');
    output->append(FastItoa((*i)->charno));
    output->append(")) ");
  }
  output->push_back(')');
}

void OpcodeProtocolRequestHandler::PrintFileResults(set<string>* matching_files,
                                                    string* output) {
  output->push_back('(');
  for (set<string>::iterator i = matching_files->begin();
       i != matching_files->end();
       ++i) {
    output->append("\"");
    output->append(*i + '"' + ' ');
  }
  output->push_back(')');
}

string OpcodeProtocolRequestHandler::EscapeQuotes(string s)  {
  string retval;

  for (int i = 0; i < s.length(); i++)
    if (s[i] == '"')
      retval.append("\\\"");
    else if (s[i] == '\\')
      retval.append("\\\\");
    else
      retval.push_back(s[i]);
  return retval;
}

SexpProtocolRequestHandler::SexpProtocolRequestHandler(bool fileindex,
                                                       bool gunzip,
                                                       string corpus_root)
    : ProtocolRequestHandler(fileindex, gunzip, corpus_root) {
  server_start_time_ = time(NULL);
  sequence_number_ = 0;

  // Initialize string->TagsCommand map

  // These string allocations seem to break the unit test when
  // heapcheck is turned on.
  tag_command_map_ = new map<string, TagsCommand>();
  (*tag_command_map_)["reload-tags-file"] = RELOAD_TAGS_FILE;
  (*tag_command_map_)["log"] = LOG;
  (*tag_command_map_)["get-server-version"] = GET_SERVER_VERSION;
  (*tag_command_map_)["get-supported-protocol-versions"]
    = GET_SUPPORTED_PROTOCOL_VERSIONS;
  (*tag_command_map_)["lookup-tag-exact"] = LOOKUP_TAG_EXACT;
  (*tag_command_map_)["lookup-tag-prefix-regexp"] = LOOKUP_TAG_PREFIX_REGEXP;
  (*tag_command_map_)["lookup-tag-snippet-regexp"] = LOOKUP_TAG_SNIPPET_REGEXP;
  (*tag_command_map_)["lookup-tags-in-file"] = LOOKUP_TAGS_IN_FILE;
  (*tag_command_map_)["load-update-file"] = LOAD_UPDATE_FILE;

  // For each client-type, we translate it into something we can put in the log
  client_code_map_ = new map<string, string>();
  (*client_code_map_)["shell"] = "sh";
  (*client_code_map_)["python"] = "py";
  (*client_code_map_)["vi"] = "vi";
  (*client_code_map_)["gnu-emacs"] = "em";
  (*client_code_map_)["xemacs"] = "em";
}

SexpProtocolRequestHandler::~SexpProtocolRequestHandler() {
  tag_command_map_->clear();
  client_code_map_->clear();

  delete client_code_map_;
  delete tag_command_map_;
}

string SexpProtocolRequestHandler::Execute(
    const char* command_list,
    TagsTable* tags_table,
    clock_t* pclock_before_preparing_results,
    struct query_profile* log) {
  DefaultTagsResultPredicate predicate;
  return Execute(command_list,
                 tags_table,
                 pclock_before_preparing_results,
                 log,
                 &predicate);
}

string SexpProtocolRequestHandler::Execute(
    const char* command_list,
    TagsTable* tags_table,
    clock_t* pclock_before_preparing_results,
    struct query_profile* log,
    const TagsResultPredicate* predicate) {
  // We first use TranslateInput to parse the query and make a
  // TagsQuery struct.
  TagsQuery query = TranslateInput(command_list,
                                   tags_table->SearchCallersByDefault());

  string output;
  // Output format:
  // ((server-start-time (T1 T2)) (sequence-number N) (value RETVAL))
  output.append("((server-start-time (");
  // Print high and low half-words of time
  output.append(FastItoa(static_cast<int64>(server_start_time_) >> 16));
  output.append(" ");
  output.append(FastItoa(static_cast<int64>(server_start_time_) & 0xffff));
  output.append(")) (sequence-number ");
  output.append(FastItoa(sequence_number_));
  output.append(") (value ");

  sequence_number_++;

  // Set the comment on the basis of the client type. We don't want to
  // use client_code_map_[...] here since that would insert a
  // key/value for any key which wasn't already in there, and clients
  // could cause us to waste arbitrary amounts of memory.
  map<string, string>::const_iterator iter =
    client_code_map_->find(query.client_type);
  if (iter == client_code_map_->end())
    log->client = "";
  else
    log->client = iter->second;
  log->command = query.command;
  log->tag = query.tag;
  log->current_file = query.file;
  log->client_message = "";

  list<const TagsTable::TagsResult*>* tag_matches = NULL;

  // Write return-value
  switch (query.command) {
    case PING:
      *pclock_before_preparing_results = clock();
      output.append(GET_FLAG(test_mode) ? "nil" : "t");
      break;
    case LOG:
      log->client_message = query.comment;
      *pclock_before_preparing_results = clock();
      output.append("t");
      break;
    case GET_SERVER_VERSION:
      *pclock_before_preparing_results = clock();
      output.append("2");
      break;
    case GET_SUPPORTED_PROTOCOL_VERSIONS:
      *pclock_before_preparing_results = clock();
      output.append("(1 2)");
      break;
    case RELOAD_TAGS_FILE:
      *pclock_before_preparing_results = clock();
      output.append(
          tags_table->ReloadTagFile(query.file, enable_gunzip_) ? "t" : "nil");
      break;
    case LOAD_UPDATE_FILE:
      *pclock_before_preparing_results = clock();
      output.append(
          tags_table->UpdateTagFile(query.file, enable_gunzip_) ? "t" : "nil");
      break;
    case LOOKUP_TAGS_IN_FILE:
      *pclock_before_preparing_results = clock();
      if (enable_fileindex_ && query.file != "") {
        tag_matches = tags_table->FindTagsByFile(StripCorpusRoot(query.file),
                                                 query.callers);
        PrintTagsResults(tag_matches, &output, predicate);
      } else {
        output.append("nil");
      }
      break;
    case LOOKUP_TAG_PREFIX_REGEXP:
      tag_matches
        = tags_table->FindRegexpTags(query.tag,
                                     StripCorpusRoot(query.file),
                                     query.callers,
                                     &query.ranking);
      *pclock_before_preparing_results = clock();
      PrintTagsResults(tag_matches, &output, predicate);
      break;
    case LOOKUP_TAG_SNIPPET_REGEXP:
      tag_matches
        = tags_table->FindSnippetMatches(query.tag,
                                         StripCorpusRoot(query.file),
                                         query.callers,
                                         &query.ranking);
      *pclock_before_preparing_results = clock();
      PrintTagsResults(tag_matches, &output, predicate);
      break;
    case LOOKUP_TAG_EXACT:
      tag_matches
        = tags_table->FindTags(query.tag,
                               StripCorpusRoot(query.file),
                               query.callers,
                               &query.ranking);
      *pclock_before_preparing_results = clock();
      PrintTagsResults(tag_matches, &output, predicate);
      break;
    default:
      *pclock_before_preparing_results = clock();
      output.append("nil");
      break;
  }

  delete tag_matches;

  output.append("))");

  return output;
}

void SexpProtocolRequestHandler::PrintTagsResults(
    list<const TagsTable::TagsResult*>* matches,
    string* output,
    const TagsResultPredicate* predicate) {
  // output format:
  // (((tag T) (snippet S) (filename F) (lineno L) (offset C)
  //            (directory-distance D)) ...)
  output->push_back('(');
  for (list<const TagsTable::TagsResult*>::const_iterator i = matches->begin();
       i != matches->end();
       ++i) {
    if (!predicate->Test(*i)) {
      continue;
    }

    output->push_back('(');
    output->append("(tag \"");
    output->append(CEscape((*i)->tag));
    output->append("\") (snippet \"");
    output->append(CEscape((*i)->linerep));
    output->append("\") (filename \"");
    output->append(CEscape((*i)->filename->Str()));
    output->append("\") (lineno ");
    output->append(FastItoa((*i)->lineno));
    output->append(") (offset ");
    output->append(FastItoa((*i)->charno));
    output->append(") (directory-distance ");
    // TODO(psung): Fill in directory-distance here
    output->append("0");
    output->append(")");

    output->append(") ");
  }
  output->push_back(')');
}

SexpProtocolRequestHandler::TagsQuery
SexpProtocolRequestHandler::TranslateInput(const char* command_list_string,
                                           bool default_callers_value) {
  SExpression* command_list = SExpression::Parse(command_list_string);
  SexpProtocolRequestHandler::TagsQuery query;

  query.client_type = "Unknown";
  query.client_version = 0;
  query.protocol_version = 2;

  query.comment = "()";

  query.command = PING;
  query.tag = "";
  query.language = "Unknown";
  query.callers = default_callers_value;
  query.file = "";

  // If we didn't get a valid s-exp, just return here with a 'ping'
  // command
  if (command_list == NULL || !command_list->IsList())
    return query;

  SExpression::const_iterator iter = command_list->Begin();

  if (iter != command_list->End()) {
    // Command name (first symbol in list) is given by iter here.
    if (iter->IsSymbol()) {
      // We don't want to use tag_command_map_[...] here since that
      // would insert a key/value for any key which wasn't already in
      // there, and clients could cause us to waste arbitrary amounts
      // of memory.
      map<string, SexpProtocolRequestHandler::TagsCommand>::const_iterator
        cmd_iter = tag_command_map_->find(iter->Repr());
      if (cmd_iter == tag_command_map_->end())
        query.command = PING;
      else
        query.command = cmd_iter->second;

      ++iter;

      // Get attribute settings
      for (; iter != command_list->End(); ++iter) {
        SExpression::const_iterator attribute_iter = iter->Begin();
        string name;

        // First item in each attribute list (name value ...) is the
        // attribute name
        if (attribute_iter->IsAtom()) {
          name = attribute_iter->Repr();
          ++attribute_iter;

          if (name == "message") {
            // Get entire message list [cdr of (message expr ...) ]
            // and store it as a string
            if (attribute_iter != iter->End())
              query.comment =
                (down_cast<const SExpressionPair*>(&(*iter)))->cdr()->Repr();
          } else {
            // Assign values to correct TagsQuery fields.
            for (; attribute_iter != iter->End(); ++attribute_iter) {
              const SExpression* value = &(*attribute_iter);

              if (value->IsList()) {
                if (name == "ranking-methods") {
                  // We only read the head of each ranking-method; the rest of
                  // the list, if it exists is ignored. They may someday be
                  // used to pass additional parameters to alter the ranking.
                  SExpression::const_iterator method_iter = value->Begin();
                  if (method_iter != value->End() && method_iter->IsSymbol()) {
                    LOG(INFO) << method_iter->Repr();
                    query.ranking.push_back(method_iter->Repr());
                  }
                }
              } else if (value->IsString()) {
                string value_str
                  = down_cast<const SExpressionString*>(value)->value();
                if (name == "language")
                  query.language = value_str;
                else if (name == "tag")
                  query.tag = value_str;
                else if (name == "file" || name == "current-file")
                  query.file = value_str;
                else if (name == "client-type")
                  query.client_type = value_str;
              } else if (value->IsInteger()) {
                int value_int
                  = down_cast<const SExpressionInteger*>(value)->value();
                if (name == "client-version")
                  query.client_version = value_int;
                else if (name == "protocol-version")
                  query.protocol_version = value_int;
              } else if (name == "callers" && !value->IsNil()) {
                query.callers = true;
              }
            }
          }
        }
      }
    }
  }

  delete command_list;

  return query;
}

LocalTagsRequestHandler::LocalTagsRequestHandler(bool fileindex, bool gunzip,
                                                 string corpus_root) {
  tags_table_ = new TagsTable(fileindex);
  sexpr_handler_ = new SexpProtocolRequestHandler(fileindex, gunzip,
                                                  corpus_root);
}

LocalTagsRequestHandler::~LocalTagsRequestHandler() {
  delete tags_table_;
  delete sexpr_handler_;
}

string LocalTagsRequestHandler::Execute(const char* command,
                                        const string& language,
                                        const string& client_path) {
  // TODO(stephenchen): It is currently meaningless to profile local tags
  // requests because there is no way to collect the data from all the users'
  // workstations and analyze them as a whole. We might want to implement a log
  // server later so that each local GTags server can upload its stats every
  // 24 hours or so.
  clock_t clock;
  struct query_profile profile;

  LanguageClientTagsResultPredicate predicate(language, client_path);
  MutexLock lock(&mu_);
  return sexpr_handler_->Execute(command, tags_table_, &clock, &profile,
                                 &predicate);
}

void LocalTagsRequestHandler::Update(const string& filename) {
  MutexLock lock(&mu_);
  tags_table_->UpdateTagFile(filename, false);
}

void LocalTagsRequestHandler::UnloadFilesInDir(const string& dirname) {
  MutexLock lock(&mu_);
  tags_table_->UnloadFilesInDir(dirname);
}
