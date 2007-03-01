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
// TagsRequestHandler stores the state associated with a TAGS file and
// uses it to convert input command requests (as strings) to outputs
// (as strings). These inputs and outputs are what are ultimately sent
// across the network, although TagsRequestHandler deals with none of
// the actual network I/O.
//
// The protocol is described at wiki/Nonconf/GTagsProtocol.

#ifndef TOOLS_TAGS_TAGSREQUESTHANDLER_H__
#define TOOLS_TAGS_TAGSREQUESTHANDLER_H__

#include "tagstable.h"
#include "tags_logger.h"

class ProtocolRequestHandler;

// Stores a TAGS file and converts protocol inputs to outputs
class TagsRequestHandler {
 public:
  // New request handler initially reading from tags_file
  TagsRequestHandler(string tags_file,
                     bool enable_fileindex,
                     bool enable_gunzip);

  ~TagsRequestHandler();

  // Takes an input query string and returns the appropriate response
  // string.
  //
  // Execute also does the following, for logging purposes:
  // -- Sets *pclock_before_preparing_results to be the clock value
  //    after the index is queried but before the results are
  //    formatted.
  // -- Fills *pcomment with any comment found in the command.
  string Execute(const char* command,
                 clock_t* pclock_before_preparing_results,
                 struct query_profile* log);

 private:
  // Currently loaded tags table
  TagsTable * tags_table_;

  // Protocol-specific handlers
  ProtocolRequestHandler* opcode_handler_;
  ProtocolRequestHandler* sexp_handler_;
};

// Superclass for protocol-specific request handlers. They satisfy
// essentially the same interface as TagsRequestHandlers but they
// don't keep their own tags table state and get it passed in on each
// call. This allows us to dispatch by protocol while only keeping a
// single tags table in memory.
class ProtocolRequestHandler {
 public:
  // fileindex specifies whether to create a per-file index of
  // tags. This requires a lot more memory.
  ProtocolRequestHandler(bool fileindex, bool gunzip)
      : enable_fileindex_(fileindex), enable_gunzip_(gunzip) { }

  // Returns the correct response string for input COMMAND using the
  // specified tags table. Update pclock and pcomment as specified by
  // TagsRequestHandler.Execute.
  virtual string Execute(const char* command,
                         TagsTable* tags_table,
                         clock_t* pclock_before_preparing_results,
                         struct query_profile* log) = 0;

 protected:
  bool enable_fileindex_;
  bool enable_gunzip_;

  // These are all the commands available.
  enum TagsCommand {
    PING = '/',
    LOG = 0,
    GET_SERVER_VERSION = 1,
    GET_SUPPORTED_PROTOCOL_VERSIONS = 2,
    RELOAD_TAGS_FILE = '!',
    LOOKUP_TAG_EXACT = ';',
    LOOKUP_TAG_PREFIX_REGEXP = ':',
    LOOKUP_TAG_SNIPPET_REGEXP = '$',
    LOOKUP_TAGS_IN_FILE = '@',
    FIND_FILE = '&'
  };
};

// Handles requests for the old protocol (1 character opcode + tag
// string)
class OpcodeProtocolRequestHandler : public ProtocolRequestHandler {
 public:
  OpcodeProtocolRequestHandler(bool fileindex, bool gunzip)
      : ProtocolRequestHandler(fileindex, gunzip) { }

  virtual string Execute(const char*, TagsTable*, clock_t*,
                         struct query_profile*);

 private:
  // Given a list of tags matches, prints them as specified by the
  // protocol and appends to output.
  void PrintTagsResults(list<const TagsTable::TagsResult*>* matches,
                        string* output);

  // Given a list of find-file matches, prints them as specified by
  // the protocol and appends to output.
  void PrintFileResults(set<string>* matching_files, string* output);

  // Replaces " in s with \" and \ with \\.
  string EscapeQuotes(string s);
};

// Handles requests for the new s-expression based protocol.
class SexpProtocolRequestHandler : public ProtocolRequestHandler {
 public:
  SexpProtocolRequestHandler(bool fileindex, bool gunzip);

  ~SexpProtocolRequestHandler();

  virtual string Execute(const char*, TagsTable*, clock_t*,
                         struct query_profile*);

 private:
  // TODO(psung): Support FIND_FILE in protocol v2

  // Stores command+parameters for a single request. All input
  // commands are translated to TagsQuery objects before they are
  // evaluated.
  struct TagsQuery {
    TagsCommand command;  // Operation to perform

    string client_type;   // Client information
    int client_version;
    int protocol_version;

    string tag;           // Identifier name or regexp
    string language;      // "c++", "java", etc.
    bool callers;
    string file;
    string comment;       // Plain string, or list of s-exps
  };

  // Given a list of tags matches, prints them as specified by the
  // protocol and appends to output.
  void PrintTagsResults(list<const TagsTable::TagsResult*>* matches,
                        string* output);

  // Converts input string to standard data
  // structure. Default_callers_value is the default value to fill in
  // for query.callers if it's not set in the command.
  TagsQuery TranslateInput(const char* command, bool default_callers_value);

  // Server start time, in seconds since the epoch
  time_t server_start_time_;
  // How many requests have previously been processed
  int sequence_number_;

  // Map to facilitate converting commands from strings to enums
  map<string, TagsCommand>* tag_command_map_;

  // Map to convert client-type field into two-char description for log
  map<string, string>* client_code_map_;
};

#endif  // TOOLS_TAGS_TAGSREQUESTHANDLER_H__
