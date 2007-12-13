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
// (See comments and description in tagstable.h.)
//
// IMPLEMENTATION NOTES
//
// We store the state of the tags file in these data structures:
//
// strings_: string table to efficiently store the strings used inside
//     all the other data structures. We guarantee that each unique
//     tag, snippet, or filename is only stored once in memory.
// fileset_: set of Filename objects representing loaded/referenced
//     files.
// map_: multimap from tag name to TagsResult*'s for all tags with
//     that name.
// filemap_: map from filename to a vector of TagsResult*'s for all tags
//     in that file. Creation of the filemap can be enabled or disabled
//     in the constructor.
// findfilemap_: multimap which maps BASE to all filenames which have
//     basename BASE.
//
// We load files by reading s-expressions sequentially from the
// file. Each item descriptor (see file format spec) generally is
// translated into a single TagsResult and is indexed in map_ and
// filemap_.
//
// We do quite a bit of s-exp processing as we load the file. Here is
// some idiomatic processing code for parsing an expression SEXP which
// is of the form (HEAD (KEY1 VALUE1) (KEY2 VALUE2) ...). We first use
// IsDeclarationWithAlist to check that the s-expression is of the
// form above. Then, on each loop iteration, we assign KEY* and VALUE*
// to attr_name and attr_value, and do something specific by
// dispatching based on the key.
//
// CHECK(IsDeclarationWithAlist(sexp, "HEAD"));
// for (SExpression::const_iterator sexp_iter = GetAttributes(sexp);
//      sexp_iter != sexp->End();
//      ++sexp_iter) {
//   SExpression::const_iterator attr_iter = sexp_iter->Begin();
//   const SExpression* attr_name = &*attr_iter;
//   ++attr_iter;
//   const SExpression* attr_value = &*attr_iter;
//   if (attr_name->Repr() == "path") {
//     CHECK(attr_value->IsString());
//     processPath(attr_value);
//   } else if ...

#include "tagstable.h"

#include <ext/hash_map>
#include <ext/hash_set>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <vector>

#include "regexp.h"
#include "tagsutil.h"
#include "tagsoptionparser.h"

DEFINE_INT32(max_results, 2000,
             "Maximum number of results to return to clients");
DEFINE_BOOL(findfile, false, "Enable file location");

DEFINE_INT32(max_snippet_size, 200,
             "Maximum snippet size (larger size snippets are truncated");
DEFINE_INT32(max_error_line,  280,
             "Maximum error line size");

TagsTable::~TagsTable() {
  FreeData();
  delete findfilemap_;
  delete filemap_;
  delete map_;
  delete fileset_;
  delete strings_;
  delete loaded_files_;
}

// Parses tags information from FILENAME, overwriting anything that
// was previously in the table. If enable_gunzip is true then filters
// the input through gunzip.
bool TagsTable::ReloadTagFile(const string& filename,
                              bool enable_gunzip) {
  LOG(INFO) << "Loading " << filename;
  FreeData();
  return LoadTagFile(filename, enable_gunzip);
}

bool TagsTable::UpdateTagFile(const string& filename,
                              bool enable_gunzip) {
  LOG(INFO) << "Updating " << filename;
  return LoadTagFile(filename, enable_gunzip);
}

bool TagsTable::LoadTagFile(const string& filename,
                              bool enable_gunzip) {
  FileReader<SExpression> filereader(filename, enable_gunzip);

  tags_comment_ = "";
  tagfile_creation_time_ = static_cast<time_t>(0);
  corpus_name_ = "";

  for (hash_map<string, bool>::iterator i = features_.begin();
       i != features_.end(); ++i) {
    i->second = false;
  }
  callers_on_by_default_ = true;

  // First expression should be the tags-format-version
  SExpression* sexp = filereader.GetNext();
  int tags_format_version = GetTagsFormatVersion(sexp);
  CHECK_EQ(tags_format_version, 2)
    << "Sorry, I don't know how to read version "
    << tags_format_version
    << " of the TAGS format.";
  delete sexp;

  // Keep track of whether any (file ...) declarations have been
  // loaded, and exit if any headers are found after the first file
  // declaration.
  bool files_loaded = false;

  while (!filereader.IsDone()) {
    sexp = filereader.GetNext();

    CHECK_NE(sexp, static_cast<SExpression*>(NULL))
      << "Expected a valid s-expression in input file.";
    CHECK(sexp->IsList()) << "Expected a declaration list at the top-level.";

    // Iterate over all elements of a single declaration.
    SExpression::const_iterator sexp_iter = sexp->Begin();

    // Read declaration type
    CHECK(sexp_iter != sexp->End())
      << "Expected a non-empty declaration at top-level.";
    CHECK(sexp_iter->IsSymbol())
      << "Expected a symbol at head of declaration.";

    if (sexp_iter->Repr() == "file") {
      // Handle file declarations
      files_loaded = true;
      ParseFileDeclaration(sexp);
    } else if (sexp_iter->Repr() == "deleted") {
      // Handle "deleted" entries, which can occur in update files
      files_loaded = true;
      ParseDeletedDeclaration(sexp);
    } else {
      // Handle header declarations (everything except 'file')
      CHECK(!files_loaded) << "Header declarations must precede "
                           << "all file declarations.";
      ParseHeaderDeclaration(sexp);
    }

    delete sexp;
  }

  LOG(INFO) << "Successfully loaded TAGS file.";

  return true;
}

const string& TagsTable::GetCommentString() const {
  return tags_comment_;
}

time_t TagsTable::GetTagfileCreationTime() const {
  return tagfile_creation_time_;
}

const string& TagsTable::GetCorpusName() const {
  return corpus_name_;
}

bool TagsTable::SearchCallersByDefault() const {
  return callers_on_by_default_;
}

list<const TagsTable::TagsResult*>* TagsTable::FindSnippetMatches(
    const string& match, const string& current_file, bool callers,
    const list<string>* ranking) const {
  list<const TagsResult*>* retval = new list<const TagsResult*>();
  int resultcount = 0;
  multimap<const char*, const TagsResult*>::const_iterator pos;
  RegExp snippetmatch(match);

  for (pos = map_->begin();
       pos != map_->end() && resultcount < GET_FLAG(max_results);
       pos++) {
    const TagsResult* tag = pos->second;
    if (snippetmatch.PartialMatch(tag->linerep)) {
      // TODO(psung): In this function and friends, we should actually
      // filter the results based on whether they were in callers or
      // not. The current code only works as long as callers and
      // definitions are always in separate files.
      retval->push_back(tag);
      resultcount++;
    }
  }

  return retval;
}

list<const TagsTable::TagsResult*>* TagsTable::FindRegexpTags(
    const string& tag, const string& current_file, bool callers,
    const list<string>* ranking) const {
  list<const TagsResult*>* retval = new list<const TagsResult*>();
  int resultcount = 0;
  multimap<const char*, const TagsResult*>::const_iterator pos;

  if (ContainsRegexpChar(tag)) {
    // Return all entries matching regexp TAG
    RegExp retag(tag);
    if (!retag.error()) {
      for (pos = map_->begin();
           pos != map_->end() && resultcount < GET_FLAG(max_results);
           pos++) {
        if (retag.FullMatch(pos->first)) {
          retval->push_back(pos->second);
          resultcount++;
        }
      }
    }
  } else {
    // Return all entries with TAG as a prefix
    for (pos = map_->lower_bound(tag.c_str());
         pos != map_->end() && resultcount < GET_FLAG(max_results)
               && IsPrefix(tag, pos->first);
         pos++) {
      retval->push_back(pos->second);
      resultcount++;
    }
  }

  return retval;
}

list<const TagsTable::TagsResult*>* TagsTable::FindTags(
    const string& tag, const string& current_file, bool callers,
    const list<string>* ranking) const {
  list<const TagsResult*>* retval = new list<const TagsResult*>();
  int resultcount = 0;

  pair<multimap<const char*, const TagsResult*>::const_iterator,
    multimap<const char*, const TagsResult*>::const_iterator> limits
    = map_->equal_range(tag.c_str());

  for (multimap<const char*, const TagsResult*>::const_iterator pos
         = limits.first;
       pos != limits.second && resultcount < GET_FLAG(max_results);
       ++pos) {
    retval->push_back(pos->second);
    resultcount++;
  }

  return retval;
}

list<const TagsTable::TagsResult*>* TagsTable::FindTagsByFile(
    const string& filename, bool callers) const {
  list<const TagsResult*>* retval = new list<const TagsResult*>();
  int resultcount = 0;
  Filename query_file(filename.c_str());

  FileMap::const_iterator pos = filemap_->find(&query_file);
  if (pos != filemap_->end()) {
    const std::vector<const TagsResult*>& results = pos->second;
    for (vector<const TagsResult*>::const_iterator i = results.begin();
         i != results.end() && resultcount < GET_FLAG(max_results);
         ++i) {
      retval->push_back(*i);
      resultcount++;
    }
  }
  return retval;
}

set<string>* TagsTable::FindFile(const string& filename) const {
  set<string>* retval = new set<string>();
  int resultcount = 0;

  pair<hash_multimap<const char*, const Filename*, hash<const char*>, StrEq>
    ::const_iterator,
    hash_multimap<const char*, const Filename*, hash<const char*>, StrEq>
    ::const_iterator> limits = findfilemap_->equal_range(filename.c_str());

  for (hash_multimap<const char*, const Filename*, hash<const char*>, StrEq>
         ::const_iterator pos = limits.first;
       pos != limits.second && resultcount < GET_FLAG(max_results);
       pos++, resultcount++) {
    retval->insert(pos->second->Str());
  }

  return retval;
}

void TagsTable::Initialize() {
  strings_ = new SymbolTable();
  fileset_ = new FileSet();
  loaded_files_ = new FileSet();
  map_ = new TagMap();
  filemap_ = new FileMap();
  findfilemap_ = new FindFileMap();
  // Register known features
  features_["callers"] = false;
}

void TagsTable::FreeData() {
  // Each TagResult appears as a value in map_ exactly once, so delete
  // all TagsResults here.
  TagMap::iterator i;
  for (i = map_->begin(); i != map_->end(); ++i) {
    delete i->second;
  }
  map_->clear();

  // The keys (strings) will be deallocated below, and the values
  // (TagsResults) have already been deleted, so just clear the map.
  filemap_->clear();

  // The keys and values are both strings and will be deallocated
  // below.
  findfilemap_->clear();

  // Deleted list of loaded files
  loaded_files_->clear();

  // Each Filename appears exactly once in this set; delete them all
  // here.
  FileSet::iterator j;
  for (j = fileset_->begin(); j != fileset_->end();) {
    // advance the iterator before we delete the element
    FileSet::iterator temp = j;
    j++;
    delete *temp;
  }
  fileset_->clear();

  // Delete the stored strings. Every string stored as a map key or
  // inside a TagsResult ought to be stored here.
  strings_->Clear();
}

void TagsTable::UnloadFilesInDir(const string& dirname) {
  FileSet::const_iterator iter;
  for (iter = fileset_->begin(); iter != fileset_->end();) {
    const Filename* filename = *iter;
    ++iter;  // Move iter before UnloadFile deletes filename from the set.
    if (HasPrefixString(filename->Str(), dirname)) {
      UnloadFile(filename);
    }
  }
}

void TagsTable::UnloadFile(const Filename* filename) {
  // No such file loaded. We are done.
  if (loaded_files_->find(filename) == loaded_files_->end()) {
    return;
  }

  LOG(INFO) << "Unloading " << filename->Str();

  // Delete from findfilemap_.
  pair<FindFileMap::iterator, FindFileMap::iterator> iter_ffm =
      findfilemap_->equal_range(filename->Basename());

  for (FindFileMap::iterator i = iter_ffm.first;
       i != iter_ffm.second;) {
    FindFileMap::iterator tmp = i;
    ++i;
    if (*(tmp->second) == *(filename)) {
      findfilemap_->erase(tmp);
    }
  }

  // Delete from map_.
  if (enable_fileindex_) {
    // Find and delete the right TagResults using filemap_.
    FileMap::iterator pos = filemap_->find(filename);
    CHECK(pos != filemap_->end());
    for (vector<const TagsResult*>::const_iterator i = pos->second.begin();
        i != pos->second.end(); ++i) {
      pair<TagMap::iterator, TagMap::iterator> iter_tag =
          map_->equal_range((*i)->tag);
      for (TagMap::iterator j = iter_tag.first;
           j != iter_tag.second;) {
        TagMap::iterator tmp = j;
        ++j;
        if (tmp->second == *i) {
          map_->erase(tmp);
          break;
        }
      }
      delete *i;
    }
    filemap_->erase(pos);
  } else {
    // If the file index is not enabled, we might still want to unload
    // files when doing an incremental update for example. To do this,
    // we need to scan through the entire table. This is fairly slow,
    // but it saves memory over maintaining the file index.
    // TODO: It might be nice to have an UnloadFiles function that unloads
    // a list of files and only scans the index once (sort the list).
    for (TagMap::iterator i = map_->begin(); i != map_->end();) {
      TagMap::iterator tmp = i;
      ++i;
      if (*(tmp->second->filename) == *(filename)) {
        delete tmp->second;
        map_->erase(tmp);
      }
    }
  }

  loaded_files_->erase(filename);
}

int TagsTable::GetTagsFormatVersion(const SExpression* sexp) {
  // Check that a valid s-expression was parsed. If we get NULL back
  // it was probably an invalid s-expression in the file.
  CHECK_NE(sexp, static_cast<SExpression*>(NULL))
    << "Expected a valid s-expression in input file.";
  // Each top-level expression found in the file must be a list.
  CHECK(sexp->IsList()) << "Expected a declaration list at the top-level.";
  // Iterate over all elements of a single declaration.
  SExpression::const_iterator sexp_iter = sexp->Begin();
  CHECK(sexp_iter != sexp->End())
    << "Expected tags-format-version declaration at file start.";
  CHECK(sexp_iter->IsSymbol())
    << "Expected tags-format-version declaration at file start.";
  CHECK_EQ(sexp_iter->Repr(), "tags-format-version")
    << "Expected tags-format-version declaration at file start.";
  ++sexp_iter;
  CHECK(sexp_iter != sexp->End())
    << "Expected a format version to follow tags-format-version.";
  CHECK(sexp_iter->IsInteger())
    << "Expected a format version to follow tags-format-version.";

  return down_cast<const SExpressionInteger*>(&*sexp_iter)->value();
}

void TagsTable::ParseHeaderDeclaration(const SExpression* sexp) {
  CHECK(sexp->IsList());
  SExpression::const_iterator sexp_iter = sexp->Begin();

  const SExpression* declaration_type = &*sexp_iter;
  ++sexp_iter;
  CHECK(sexp_iter != sexp->End())
    << "Expected parameter(s) to follow declaration type.";
  const SExpression* declaration_value = &*sexp_iter;

  // In general, just read the value and stick it into the
  // appropriate member variable.
  if (declaration_type->Repr() == "tags-comment") {
    CHECK(declaration_value->IsString())
      << "Expected string after tags-comment.";
    tags_comment_ =
      down_cast<const SExpressionString*>(declaration_value)->value();
  } else if (declaration_type->Repr() == "tags-corpus-name") {
    CHECK(declaration_value->IsString())
      << "Expected string after tags-corpus-name.";
    corpus_name_ =
      down_cast<const SExpressionString*>(declaration_value)->value();
  } else if (declaration_type->Repr() == "timestamp") {
    // TODO(psung): Parse and fill in the timestamp here. At present
    // we don't allocate enough space to do this.  (SExpression ints
    // are capped at 2^31-1 on 32-bit machines.)
  } else if (declaration_type->Repr() == "features") {
    CHECK(declaration_value->IsList()) << "Expected a list after features.";

    for (SExpression::const_iterator features_iter
           = declaration_value->Begin();
         features_iter != declaration_value->End();
         ++features_iter) {
      CHECK(features_iter->IsSymbol())
        << "Expected symbol in feature list.";

      hash_map<string, bool>::iterator find_iter =
        features_.find(features_iter->Repr());
      if (find_iter != features_.end())
        find_iter->second = true;
      else
        LOG(INFO) << "feature list contained unrecognized feature name: "
                  << features_iter->Repr();
    }
  } else {
    LOG(INFO) << "File header contained unrecognized declaration type: "
              << declaration_type->Repr();
  }
}

void TagsTable::ParseDeletedDeclaration(const SExpression* sexp) {
  CHECK(sexp->IsList());
  SExpression::const_iterator sexp_iter = sexp->Begin();
  const SExpression* declaration_type = &*sexp_iter;
  ++sexp_iter;
  CHECK_EQ(declaration_type->Repr(), "deleted");
  CHECK(sexp_iter != sexp->End())
    << "Expected parameter(s) to follow declaration type.";
  const SExpression* declaration_value = &*sexp_iter;
  CHECK(declaration_value->IsString())
    << "Expected string after deleted declaration.";
  const Filename* filename = NULL;
  filename = FileGet(
    down_cast<const SExpressionString*>(declaration_value)->value());
  CHECK_NE(filename, static_cast<Filename*>(NULL))
    << "Expected a file path inside deleted declaration.";
  UnloadFile(filename);
}

void TagsTable::ParseFileDeclaration(const SExpression* sexp) {
  const Filename* filename = NULL;
  const char* language = "";
  const SExpression* contents_list = NULL;

  CHECK(IsDeclarationWithAlist(sexp, "file"));

  // Extract attributes from file declaration
  for (SExpression::const_iterator file_iter = GetAttributes(sexp);
       file_iter != sexp->End();
       ++file_iter) {
    SExpression::const_iterator attr_iter = file_iter->Begin();
    const SExpression* attr_name = &*attr_iter;
    ++attr_iter;
    const SExpression* attr_value = &*attr_iter;

    if (attr_name->Repr() == "path") {
      CHECK(attr_value->IsString());
      filename = FileGet(
          down_cast<const SExpressionString*>(attr_value)->value());
    } else if (attr_name->Repr() == "language") {
      CHECK(attr_value->IsString());
      language = strings_->Get(
          down_cast<const SExpressionString*>(attr_value)->value().c_str());
    } else if (attr_name->Repr() == "contents") {
      CHECK(attr_value->IsList());
      contents_list = &*attr_value;
    } else {
      LOG(INFO) << "file declaration contained unrecognized attribute name: "
                << attr_name->Repr();
    }
  }

  // Check that all fields were assigned
  CHECK_NE(filename, static_cast<Filename*>(NULL))
    << "Expected a file path inside the file declaration.";
  CHECK_STRNE(language, "")
    << "Expected a file language inside the file declaration.";
  CHECK_NE(contents_list, static_cast<SExpression*>(NULL))
    << "Expected a contents list inside the file declaration.";

  LOG(INFO) << "Processing " << filename->Str();

  UnloadFile(filename);

  // Add into loaded_files_
  loaded_files_->insert(filename);

  vector<const TagsResult*> tags_vector;

  // Iterate through contents
  for (SExpression::const_iterator lineitem_iter = contents_list->Begin();
       lineitem_iter != contents_list->End();
       ++lineitem_iter) {
    TagsResult* tag = ParseItemDeclaration(&*lineitem_iter, filename);

    // If we got tag == NULL and no further work is
    // needed.
    if (tag == NULL)
      continue;

    tag->filename = filename;
    tag->language = language;

    map_->insert(make_pair(tag->tag, tag));
    if (enable_fileindex_)
      tags_vector.push_back(tag);
    if (GET_FLAG(findfile))
      findfilemap_->insert(make_pair(tag->filename->Basename(), tag->filename));

    if (tag->type != CALL)
      callers_on_by_default_ = false;

    LOG_EVERY_N(INFO, 100000) << "Tag: " << tag->tag << "\n"
                              << "Snippet: " << tag->linerep << "\n"
                              << "Filename: " << tag->filename->Str() << "\n"
                              << "Lineno: " << tag->lineno << "\n"
                              << "Charno: " << tag->charno;
  }

  if (enable_fileindex_) {
    tags_vector.resize(tags_vector.size());
    filemap_->insert(make_pair(filename, tags_vector));
  }
}

TagsTable::TagsResult* TagsTable::ParseItemDeclaration(
    const SExpression* sexp, const Filename* filename) {
  CHECK(IsDeclarationWithAlist(sexp, "item"));

  TagsResult* retval = NULL;

  int lineno = 0;
  int charno = 0;
  const char* snippet = NULL;

  // Extract attributes from item declaration
  for (SExpression::const_iterator item_iter = GetAttributes(sexp);
       item_iter != sexp->End();
       ++item_iter) {
    SExpression::const_iterator attr_iter = item_iter->Begin();
    const SExpression* attr_name = &*attr_iter;
    ++attr_iter;
    const SExpression* attr_value = &*attr_iter;

    if (attr_name->Repr() == "line") {
      CHECK(attr_value->IsInteger());
      lineno = down_cast<const SExpressionInteger*>(attr_value)->value();
    } else if (attr_name->Repr() == "offset") {
      CHECK(attr_value->IsInteger());
      charno = down_cast<const SExpressionInteger*>(attr_value)->value();
    } else if (attr_name->Repr() == "descriptor") {
      retval = ParseDescriptorDeclaration(attr_value, filename);
    } else if (attr_name->Repr() == "snippet") {
      CHECK(attr_value->IsString());
      string snippet_str
        = down_cast<const SExpressionString*>(attr_value)->value();
      if (snippet_str.size() > GET_FLAG(max_snippet_size))
        snippet_str.resize(GET_FLAG(max_snippet_size));
      snippet = strings_->Get(snippet_str.c_str());
    }
  }

  // Assign values to TagsResult struct
  if (retval != NULL) {
    retval->lineno = lineno;
    retval->charno = charno;
    // If snippet wasn't specified, we should set it to an empty string.
    if (snippet == NULL)
      retval->linerep = strings_->Get("");
    else
      retval->linerep = snippet;
  }

  return retval;
}

TagsTable::TagsResult* TagsTable::ParseDescriptorDeclaration(
    const SExpression* sexp, const Filename* filename) {

  const SExpression* descriptor_head = &*(sexp->Begin());
  TagsResult* retval = new TagsResult;

  if (descriptor_head->Repr() == "call") {
    // Handle references
    CHECK(IsDeclarationWithAlist(sexp, "call"));
    // Extract values from call declaration
    for (SExpression::const_iterator descriptor_iter = GetAttributes(sexp);
         descriptor_iter != sexp->End();
         ++descriptor_iter) {
      SExpression::const_iterator attr_iter = descriptor_iter->Begin();
      const SExpression* attr_name = &*attr_iter;
      ++attr_iter;
      const SExpression* attr_value = &*attr_iter;

      if (attr_name->Repr() == "to")
        retval->tag = strings_->Get(GetTagNameFromRef(attr_value).c_str());
    }
  } else {
    // All other tag definition types
    if (descriptor_head->Repr() == "type")
      retval->type = TYPE_DEFN;
    else if (descriptor_head->Repr() == "function")
      retval->type = FUNCTION_DEFN;
    else if (descriptor_head->Repr() == "variable")
      retval->type = VARIABLE_DEFN;
    else if (descriptor_head->Repr() == "generic-tag")
      retval->type = GENERIC_DEFN;
    else
      LOG(FATAL) << "Unexpected descriptor type encountered."
                 << descriptor_head->Repr();

    // TODO(psung): At present we don't know how to do special
    // handling for types, functions, and variables. We just read the
    // 'tag' field as if they were generic tags.
    CHECK(IsDeclarationWithAlist(sexp, descriptor_head->Repr()));

    for (SExpression::const_iterator descriptor_iter = GetAttributes(sexp);
         descriptor_iter != sexp->End();
         ++descriptor_iter) {
      SExpression::const_iterator attr_iter = descriptor_iter->Begin();
      const SExpression* attr_name = &*attr_iter;
      ++attr_iter;
      const SExpression* attr_value = &*attr_iter;

      if (attr_name->Repr() == "tag") {
        CHECK(attr_value->IsString());
        retval->tag = strings_->Get(
            down_cast<const SExpressionString*>(attr_value)->value().c_str());
      }
    }
  }

  CHECK_NE("", retval->tag) << "Expected non-empty tag name.";

  return retval;
}

const string& TagsTable::GetTagNameFromRef(const SExpression* sexp) const {
  CHECK(IsDeclarationWithAlist(sexp, "ref"));

  const string* p_return = NULL;

  for (SExpression::const_iterator ref_iter = GetAttributes(sexp);
       ref_iter != sexp->End();
       ++ref_iter) {
    SExpression::const_iterator attr_iter = ref_iter->Begin();
    const SExpression* attr_name = &*attr_iter;
    ++attr_iter;
    const SExpression* attr_value = &*attr_iter;

    if (attr_name->Repr() == "name") {
      CHECK(attr_value->IsString());
      p_return = &(down_cast<const SExpressionString*>(attr_value)->value());
    }
  }

  CHECK_NE(p_return, static_cast<string*>(NULL))
    << "Expected name inside reference";
  CHECK_NE(*p_return, "")
    << "Expected name inside reference";
  return *p_return;
}

bool TagsTable::IsDeclarationWithAlist(const SExpression* sexp,
                                       const string& decltype) const {
  if (!sexp->IsList()) {
    LOG(WARNING) << "Expected expression to be a list.";
    return false;
  }

  SExpression::const_iterator sexp_iter = sexp->Begin();

  // Check that the first element is a symbol
  if (sexp_iter == sexp->End()) {
    LOG(WARNING) << "Expected expression to be a non-empty list.";
    return false;
  }
  if (!sexp_iter->IsSymbol()) {
    LOG(WARNING) << "Expected expresssion to begin with a symbol.";
    return false;
  }
  if (down_cast<const SExpressionSymbol*>(&*sexp_iter)->name() != decltype) {
    LOG(WARNING) << "Expected expression to begin with symbol: "
                 << decltype;
    return false;
  }

  ++sexp_iter;

  // Check the attributes
  for (; sexp_iter != sexp->End(); ++sexp_iter) {
    if (!sexp_iter->IsList()) {
      LOG(WARNING) << "Expected attribute-value sets to be lists.";
      return false;
    }

    // TODO(psung): Maybe reject the s-expression if there are
    // duplicate names in the attribute list

    SExpression::const_iterator attr_iter = sexp_iter->Begin();
    // Check that each attribute has two elements, first of which is a
    // symbol
    if (attr_iter == sexp_iter->End() || !attr_iter->IsSymbol()) {
      LOG(WARNING) << "Expected first element of attribute-value set to be "
                   << "a symbol.";
      return false;
    }
    ++attr_iter;
    if (attr_iter == sexp_iter->End()) {
      LOG(WARNING) << "Expected attribute-value set to contain second element.";
      return false;
    }
    ++attr_iter;
    if (attr_iter != sexp_iter->End()) {
      LOG(WARNING) << "Expected attribute-value set to contain only "
                   << "two elements.";
      return false;
    }
  }

  return true;
}

SExpression::const_iterator TagsTable::GetAttributes(
    const SExpression* sexp) const {
  // All this really does is return an iterator starting from the
  // second element
  SExpression::const_iterator sexp_iter = sexp->Begin();
  ++sexp_iter;
  return sexp_iter;
}

bool TagsTable::IsPrefix(const string& a, const string& b) const {
  return (a.size() <= b.size()) && !(b.compare(0, a.size(), a));
}

bool TagsTable::ContainsRegexpChar(const string& tag) const {
  static RegExp rechars("[^a-zA-Z0-9\\-_]");
  return rechars.PartialMatch(tag);
}

const Filename* TagsTable::FileGet(const string& file_str) {
  Filename file(file_str.c_str(), strings_);
  FileSet::const_iterator iter = fileset_->find(&file);

  if (iter != fileset_->end())
    return *iter;

  Filename* f = new Filename(file);
  fileset_->insert(f);
  return f;
}
