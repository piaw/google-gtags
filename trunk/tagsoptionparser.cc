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

#include <cstdlib>
#include <iostream>

#include "tagsoptionparser.h"

namespace tools_tags_tagsoptionparser {
StringOptions & string_options() {
  static StringOptions options;
  return options;
}

IntegerOptions & integer_options() {
  static IntegerOptions options;
  return options;
}

BooleanOptions & boolean_options() {
  static BooleanOptions options;
  return options;
}

}

namespace {
string usage_message;
}

using namespace tools_tags_tagsoptionparser;

// Iterate through all the flags and print help messages
void PrintUsage() {
  for (StringOptions::const_iterator i = string_options().begin();
       i != string_options().end(); ++i) {
    cerr << "\t-" << i->first << " (" << i->second.second
              << ") type: string default: " << i->second.first << endl;
  }

  for (IntegerOptions::const_iterator i = integer_options().begin();
       i != integer_options().end(); ++i) {
    cerr << "\t-" << i->first << " (" << i->second.second
              << ") type: int32 default: " << i->second.first << endl;
  }

  for (BooleanOptions::const_iterator i = boolean_options().begin();
       i != boolean_options().end(); ++i) {
    cerr << "\t-" << i->first << " (" << i->second.second
              << ") type: bool default: " << i->second.first << endl;
  }

}

// Parse commandline arguements. Accepts the following formats:
//   -option=value
//   --option=value
//   -boolean_option
//   --boolean_option
//   -non_boolean_option value
//   --non_boolean_option value
void ParseArgs(int argc, char ** argv) {
  cerr << boolean_options().size() << endl;
  int i = 1; //argv[0] is the command name
  char buf[256];
  while (i < argc) {
    if (*argv[i] == '-') {
      // skip '-'
      const char * option_name = argv[i++] + 1;
      // we also accept --longoption
      // if option has another leading '-', skip that one too
      if (*option_name == '-') {
        option_name++;
      }

      // see if option_name is in form of name=arg
      // if so, separate them and point arg to the argument
      const char * arg = 0;
      const char * counter = option_name;
      while (*counter) {
        if (*counter == '=') {
          // copy the name to a buffer so we can null terminate it.
          // we can't do it in place because we don't want to modify
          // parameters
          strncpy(buf, option_name, counter - option_name);
          buf[counter - option_name] = '\0'; // null terminate name here
          option_name = buf;
          arg = counter+1;
          break;
        }
        counter++;
      }

      //option should be in one of the 3 sets
      StringOptions::iterator iter_str = string_options().find(option_name);
      if (iter_str != string_options().end()) {
        if (arg) { //if we already extracted the argument, use it
          iter_str->second.first = string(arg);
        } else {
          if (i >= argc) {
            // missing argument
            PrintUsage();
            exit(-1);
          }
          iter_str->second.first = string(argv[i++]);
        }
        continue;
      }

      IntegerOptions::iterator iter_int = integer_options().find(option_name);
      if (iter_int != integer_options().end()) {
        if (arg) {
          iter_int->second.first = atoi(arg);
        } else {
          if (i >= argc) {
            PrintUsage();
            exit(-1);
          }
          iter_int->second.first = atoi(argv[i++]);
        }
        continue;
      }

      BooleanOptions::iterator iter_bool = boolean_options().find(option_name);
      if (iter_bool != boolean_options().end()) {
        // for boolean flags, if there is an argument, set it
        // if there is no argument, set it to true
        if (arg) {
          if (strcmp(arg, "true") == 0 || strcmp(arg, "True") == 0 ||
              strcmp(arg, "TRUE") == 0) {
            iter_bool->second.first = true;
          } else {
            //assume false
            iter_bool->second.first = false;
          }
        } else {
          iter_bool->second.first = true;
        }
        continue;
      }

      // If we get here, then we can't recognize the option
      // exit
      cerr << "ERROR: unknown command line flag '"
                << option_name << "'" << endl;
      exit(-1);
    } else {
      // Arguments are in wrong format
      PrintUsage();
      exit(-1);
    }
  };
}

void SetUsage(const char * message) {
  usage_message = message;
}

void ShowUsage(const char * program_name) {
  cerr << usage_message << endl;
  PrintUsage();
}
