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
// Authors: psung@google.com (Phil Sung), piaw@google.com (Piaw Na)
//
// Stores a representation of a TAGS database in memory. Provides
// TagsTable::ReloadTagFile to load from disk (see
// wiki/Nonconf/GTagsTagsFormat for a specification of the file
// format) and provides an interface for searching tag information.
//
// (See implementation notes in tagstable.cc.)

#ifndef TOOLS_TAGS_TAGSTABLE_H__
#define TOOLS_TAGS_TAGSTABLE_H__

#include <list>
#include <map>
#include <set>
#include <string>
#include <ext/hash_map>

#include "filename.h"
#include "symboltable.h"
#include "sexpression.h"

class TagsTable {
 public:
  // Default constructor disables file-index.
  TagsTable() : enable_fileindex_(false) {
    Initialize();
  }

  // Conditionally enables file-index based on argument.
  explicit TagsTable(bool enable_fileindex)
      : enable_fileindex_(enable_fileindex) {
    Initialize();
  }

  virtual ~TagsTable();

  // One of these is stored with each TagsResult, to store the type of
  // the entry.
  enum TagType {
    CALL,           // Not a tag definition, just a reference
    GENERIC_DEFN,   // Definition of unknown kind (type, variable, or function)
    TYPE_DEFN,
    VARIABLE_DEFN,
    FUNCTION_DEFN
  };

  // Stores data associated with a single instance of a tag. Each item
  // descriptor in a file has an associated TagsResult object.
  struct TagsResult {
    TagType type : 16;         // type of tag
    int charno;                // char offset of beginning of line
    int lineno;                // line number
    const char* tag;           // tag
    const char* linerep;       // text of line the tag appears on
    const Filename* filename;  // which file
    const char* language;      // file language
  };

  // Load the tag file from FILENAME. The file format is described at
  // wiki/Nonconf/GTagsTagsFormat.
  bool ReloadTagFile(const string& filename, bool enable_gunzip);

  // Update the tag file from FILENAME. Only effects entries from files
  // listed in the input file.
  bool UpdateTagFile(const string& filename, bool enable_gunzip);

  // Accessors to retrieve metadata for the tagsfile.
  const string& GetCommentString() const;
  time_t GetTagfileCreationTime() const;
  const string& GetCorpusName() const;

  bool SearchCallersByDefault() const;

  // These functions are used to query the TagsTable. CURRENT_FILE, if
  // not "", is used to rank the results. Each returns a newly
  // allocated data structure.

  // Return snippet matches
  virtual list<const TagsResult*>* FindSnippetMatches(
      const string& match, const string& current_file, bool callers,
      const list<string>* ranking) const;
  // Return regexp matches
  virtual list<const TagsResult*>* FindRegexpTags(
      const string& tag, const string& current_file, bool callers,
      const list<string>* ranking) const;
  // Return matching tags
  virtual list<const TagsResult*>* FindTags(
      const string& tag, const string& current_file, bool callers,
      const list<string>* ranking) const;

  // Return all tags in a particular file
  list<const TagsResult*>* FindTagsByFile(const string& filename,
                                          bool callers) const;
  // Return all files that have the correct base name
  set<string>* FindFile(const string& filename) const;

  // Unload all files contained in dir.
  void UnloadFilesInDir(const string& dirname);
 private:
  // Instantiates all needed members. Should be called only once, from
  // the constructor.
  void Initialize();

 protected:
  // Clears all data structures and deallocates stored strings, in
  // preparation for destructor or loading a new TAGS file.
  virtual void FreeData();

  // Load the tag file from FILENAME. The file format is described at
  // wiki/Nonconf/GTagsTagsFormat.
  bool LoadTagFile(const string& filename, bool enable_gunzip);

  // Unload all tags from the specified file.
  virtual void UnloadFile(const Filename* filename);

  // Helpers for parsing s-expression input from file:

  // CHECKs that SEXP is a valid (tags-format-version ...)
  // declaration, and returns the version given.
  int GetTagsFormatVersion(const SExpression* sexp);
  // If SEXP is a valid header declaration other than
  // tags-format-version, parses it and updates the TagsTable.
  void ParseHeaderDeclaration(const SExpression* sexp);
  // If SEXP is a valid (file ...) declaration, parses it and updates
  // the TagsTable.
  void ParseFileDeclaration(const SExpression* sexp);
  // Parses SEXP, which must be a valid (item ...) declaration. Returns
  // a newly allocated TagsResult with the type, tag, snippet, lineno,
  // and charno fields filled in. FILENAME should be the path to the file
  // containing the item declaration. If SEXP does not represent a tag,
  // returns NULL.
  virtual TagsResult* ParseItemDeclaration(const SExpression* sexp,
                                           const Filename* filename);
  // Parses SEXP, which must be a valid item descriptor. Rreturns a newly
  // allocated TagsResult with the type and tag fields filled in. FILENAME
  // should be the path to the file containing the descriptor. If SEXP does
  // not represent a tag, returns NULL.
  virtual TagsResult* ParseDescriptorDeclaration(const SExpression* sexp,
                                                 const Filename* filename);
  // If SEXP is a valid (deleted ...) declaration, parses it and updates
  // the TagsTable.
  void ParseDeletedDeclaration(const SExpression* sexp);
  // If sexp is of the form (ref (name "TAGNAME") (id ...)), returns
  // the value of its NAME attribute as a string.
  const string& GetTagNameFromRef(const SExpression* sexp) const;
  // Returns true if SEXP is of the form (DECLTYPE (name value) ...).
  //
  // We frequently use s-expressions of this form to store
  // attribute-value pairs attached to an entity (file, line, etc.)
  bool IsDeclarationWithAlist(const SExpression* sexp,
                              const string& decltype) const;
  // Returns an iterator over the attributes of SEXP (everything
  // except the first item). Returns a suffix of SEXP, so that the end
  // of the list of attributes is still given by sexp->End().
  SExpression::const_iterator GetAttributes(const SExpression* sexp) const;

  // Returns true if a is a prefix of b
  bool IsPrefix(const string& a, const string& b) const;

  bool ContainsRegexpChar(const string& tag) const;

  // Analagous to SymbolTable::Get; ensures that we only store each
  // unique Filename once. Given a path FILE_STR, returns a pointer to
  // a Filename inside fileset_ which represents the same
  // path. Allocates a new Filename if necessary.
  const Filename* FileGet(const string& file_str);

  // Provide a string operator< for sorted string containers
  class StrCompare {
   public:
    bool operator()(const char* s1, const char* s2) const {
      return strcmp(s1, s2) < 0;
    }
  };

  // Provide a string operator== for hashed string containers
  class StrEq {
   public:
    bool operator()(const char* s1, const char* s2) const {
      return !strcmp(s1, s2);
    }
  };

  // Provide Filename* operator== and hasher for hashed Filename*
  // containers (hashes and compares on Filename value, not pointer
  // value)
  class FileHash {
   public:
    size_t operator()(const Filename* s1) const {
      return hasher_(s1->Str().c_str());
    }
   private:
    hash<const char*> hasher_;
  };

  class FileEq {
   public:
    bool operator()(const Filename* s1, const Filename* s2) const {
      return *s1 == *s2;
    }
  };

  // Map and set types for our data structures.
  typedef hash_set<const Filename*, FileHash, FileEq> FileSet;
  typedef multimap<const char*, const TagsResult*, StrCompare> TagMap;
  typedef hash_map<const Filename*, vector<const TagsResult*>, FileHash, FileEq>
    FileMap;
  typedef hash_multimap<const char*, const Filename*, hash<const char*>, StrEq>
    FindFileMap;

  // TODO(psung): Storing callers and non-callers in separate data
  // structures might make lookups faster, since we never search for
  // both and always have to filter one of them out.

  // Store all the strings that we use here
  SymbolTable* strings_;
  // Set of all filenames
  FileSet* fileset_;
  // Set of all currently indexed filenames
  FileSet* loaded_files_;
  // Index all tagged lines by tagname. This needs to be not hashed so
  // we can do range queries when we're looking for prefixes.
  TagMap* map_;
  // Index all tagged lines by filename
  FileMap* filemap_;
  // Index all files by their basename
  FindFileMap* findfilemap_;
  // Allow searching for tags by filename. This can be a big space hit.
  bool enable_fileindex_;

  // Tagsfile metadata
  string tags_comment_;
  time_t tagfile_creation_time_;
  string corpus_name_;

  // Features
  hash_map<string, bool> features_;

  // This is to make the transition from etags to new-style tags
  // easier. Under etags, there was no distinction in the file format
  // between tag definition lines and tag caller lines-- whichever one
  // you got was solely based on what server you queried. The
  // new-style tags format supports putting both callers and
  // definitions in the same file, and it's up to the client to ask
  // for one or the other with '(callers t|nil), with nil being the
  // default.
  //
  // However, for performance and continuity reasons, one might still
  // choose to segregate the definition and callers files/servers:
  //
  // 1. Serving callers is more CPU intensive, and we don't want
  //    callers requests to slow down definitions requests.
  //
  // 2. At present we provide a utility which converts a TAGS file to
  //    a new-style file with only definitions (generic-tag ...) and
  //    converts a TAGSC file to a new-style file with only callers
  //    (ref ...).
  //
  // As a consequence, if a sufficiently old client (one who didn't
  // provide 'callers) were to go to a new callers server, it would
  // get no query results since the callers server would only look for
  // (generic-tag ...) descriptors by default, and there wouldn't be
  // any.
  //
  // To avoid breaking this setup for old clients, we detect when the
  // file contains no definition lines and assume that it's a
  // callers-only file, set callers_on_by_default_ = true, and default
  // to showing callers information instead of definition information.
  bool callers_on_by_default_;
};

#endif  // TOOLS_TAGS_TAGSTABLE_H__
