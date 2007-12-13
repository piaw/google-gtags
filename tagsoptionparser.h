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
// TagsOptionParser parses commandline options. Options can be declared in any
// .cc file using DEFINE_STRING, DEFINE_BOOL and DEFINE_INT32 macros.The flags
// can be later accessed using GET_FLAG(flagname). Client code should call
// ParseArgs in its main function before flags are read.
//
// If GTAGS_GOOGLE_INTERNAL is defined, TagsOptionParser delegates all the tasks
// to macros defined in base/commandline.h
//
// If GTAGS_GOOGLE_INTERNAL is not defined, then it creates three maps in the
// form of map<name, pair<value, help> >. Each call to DEFINE_TYPE macro will
// generate a static initiation of an FlagRegister object which will regiester
// the flag in an approperiate map. The macro creates FlagRegister in
// FlagName[optionname] namespace to avoid name conlision. It also set a pointer
// to the associated map in the same namespace.
//
// When GET_FLAG(flagname) is called, the macro goes into FlagName[flagname]
// namespace, retrieve the associated map, look up the option from the map and
// returns its value. A call to GET_FLAG with a flagname that has not been
// defined using DEFINE_TYPE macro will result in a compilation error.


#ifndef TOOLS_TAGS_TAGSOPTIONPARSER_H__
#define TOOLS_TAGS_TAGSOPTIONPARSER_H__

#include <ext/hash_map>
#include <cstring>
#include <list>
#include <string>
#include <utility>

#include <iostream>

#include "tagsutil.h"

// Wrap internal/helper class and variables in a namespace
// client code shouldn't need to look into this namespace
namespace tools_tags_tagsoptionparser {

// String comparator
struct strCmp {
  bool operator()( const char* s1, const char* s2 ) const {
    return strcmp( s1, s2 ) == 0;
  }
};

// Type aliases to save typing
// An option map is in general form: map<name, pair<value, help> >
typedef hash_map<const char *, pair<string, const char*>,
                 hash<const char *>, strCmp> StringOptions;
typedef hash_map<const char *, pair<list<string>, const char*>,
                 hash<const char *>,  strCmp> MultiStringOptions;
typedef hash_map<const char *, pair<int, const char*>,
                 hash<const char *>, strCmp> IntegerOptions;
typedef hash_map<const char *, pair<bool, const char*>,
                 hash<const char *>,  strCmp> BooleanOptions;

// Getter functions for global option registries.
// We need to wrap them as static variables in getter functions to ensure that
// they will be fully constructed before they are used by static FlagRegister
StringOptions & string_options();
MultiStringOptions & multistring_options();
IntegerOptions & integer_options();
BooleanOptions & boolean_options();

// FlagRegister helps us run code before program enters main().
// We statically create FlagRegister objects and their constructors will
// register various flags with the maps we declared above.
class FlagRegister {
 public:
  FlagRegister(const char * arg,
               const char * default_value,
               const char * help) {
    string_options()[arg] = pair<string, const char*>(default_value, help);
  }

  FlagRegister(const char * arg,
               list<string> default_value,
               const char * help) {
    multistring_options()[arg] =
        pair<list<string>, const char*>(default_value, help);
  }

  FlagRegister(const char * arg, int default_value, const char * help) {
    integer_options()[arg] = pair<int, const char*>(default_value, help);
  }

  FlagRegister(const char * arg, bool default_value, const char * help) {
    boolean_options()[arg] = pair<bool, const char*>(default_value, help);
  }
};

} //end namespace


// Macros for defining commandline options
// When calling a DEFINE_TYPE macro, it creates a FlagRegister object in a
// separate namespace to register the flag with the maps. It also keeps a
// pointer to the approperiate map in the same namespace. The pointer is
// accessed later in GET_FLAG.
#undef DEFINE_STRING
#define DEFINE_STRING(arg, default_value, help)                               \
namespace Flag_Name_##arg {                                                   \
static tools_tags_tagsoptionparser::FlagRegister flag_register(#arg,          \
                                                               default_value, \
                                                               help);         \
tools_tags_tagsoptionparser::StringOptions * registry =                       \
    &tools_tags_tagsoptionparser::string_options();                           \
}

#undef DEFINE_MULTISTRING
#define DEFINE_MULTISTRING(arg, default_value, help)                          \
namespace Flag_Name_##arg {                                                   \
static tools_tags_tagsoptionparser::FlagRegister flag_register(#arg,          \
                                                               default_value, \
                                                               help);         \
tools_tags_tagsoptionparser::MultiStringOptions * registry =                  \
    &tools_tags_tagsoptionparser::multistring_options();                      \
}

#undef DEFINE_BOOL
#define DEFINE_BOOL(arg, default_value, help)                                 \
namespace Flag_Name_##arg {                                                   \
static tools_tags_tagsoptionparser::FlagRegister flag_register(#arg,          \
                                                               default_value, \
                                                               help);         \
tools_tags_tagsoptionparser::BooleanOptions * registry =                      \
    &tools_tags_tagsoptionparser::boolean_options();                          \
}

#undef DEFINE_INT32
#define DEFINE_INT32(arg, default_value, help)                                \
namespace Flag_Name_##arg {                                                   \
static tools_tags_tagsoptionparser::FlagRegister flag_register(#arg,          \
                                                               default_value, \
                                                               help);         \
tools_tags_tagsoptionparser::IntegerOptions * registry =                      \
    &tools_tags_tagsoptionparser::integer_options();                          \
}


// Macros for declaring commandline options
// When calling a DECLARE_TYPE macro, it creates a reference to an external
// FlagRegister, one that would be created with a DEFINE_TYPE macro.
#undef DECLARE_STRING
#define DECLARE_STRING(arg)                                             \
namespace Flag_Name_##arg {                                             \
  extern tools_tags_tagsoptionparser::StringOptions * registry;         \
}

#undef DECLARE_MULTISTRING
#define DECLARE_MULTISTRING(arg)                                        \
namespace Flag_Name_##arg {                                             \
  extern tools_tags_tagsoptionparser::MultiStringOptions * registry;    \
}

#undef DECLARE_BOOL
#define DECLARE_BOOL(arg)                                               \
namespace Flag_Name_##arg {                                             \
  extern tools_tags_tagsoptionparser::BooleanOptions * registry;        \
}

#undef DECLARE_INT32
#define DECLARE_INT32(arg)                                              \
namespace Flag_Name_##arg {                                             \
  extern tools_tags_tagsoptionparser::IntegerOptions * registry;        \
}


// Returns the value for a given flag.
// It goes into associated namespace to determine which map to query and returns
// the value of the flag. If client code calls this macro with a flagname that
// has not been defined, a compile time error is generated as there'd be no
// namespace assoicated with the flag.
#undef GET_FLAG
#define GET_FLAG(arg) (*(Flag_Name_##arg::registry))[#arg].first


// The following functions should be defined for both internal or opernsource
// version

// Call this function to parse command line arguments
void ParseArgs(int argc, char ** argv);

// Set usage flag
// Parameter is a description of the general usage of the program
void SetUsage(const char *);

// Print usage message with all the flags
// Parameter is the name of the program
void ShowUsage(const char *);

#endif  // TOOLS_TAGS_TAGSOPTIONPARSER_H__
