#!/bin/bash
# Copyright 2004-2007 Google Inc.
# All Rights Reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
# Author: Sitaram Iyer <sitaram@google.com>

# Command-line interface to gtags.
# (tip: may be convenient to alias 't' to gtags.sh)
#
# Example usage:                   gtags.sh -w -r mapreduce Output
# which is equivalent to:          gtags.sh --color ^Output$ | grep \ mapreduce/
#
# For invocations:                 gtags.sh -i -w -r mapreduce Output
# Emit vi cmds for cut-and-paste:  gtags.sh -v -w -r mapreduce Output


function usage() {
cat <<EOF
Command-line interface to gtags.

Usage: gtags.sh [flags] tag ...
  Shows lines containing a definition for tag.

flags:
  -L|--lang c++|java|python	# language of source files to look in
  -w|--word			# match whole word only (same as ^tag$)
  -i|--invocations		# show invocations of tag, not definitions
  -r|--restrict <dir>	        # restrict matches to <dir>/
  --color | --nocolor		# override "use ansi colors if isatty(stdout)"
  --basedir <prefix>            # dir to prefix
  -p|--plain			# output file:line for each match
  -l|--less			# output a less command for each match
  -v|--vi 			# output a vi command for each match
                                # -p, -l, and -v are mutually exclusive
  --status                      # check server status

(tip: may be convenient to alias 't' to gtags.sh)

Example usage:                   gtags.sh -w -r mapreduce Output
which is equivalent to:          gtags.sh --color ^Output$ | grep \ mapreduce/

For invocations:                 gtags.sh -i -w -r mapreduce Output
Emit vi cmds for cut-and-paste:  gtags.sh -v -w -r mapreduce Output

Default values for most options can be set by defining environnent variables
starting with \`\`GTAGS_''.
  The default corpus can be set by defining \$GTAGS_CORPUS.
  The default language can be set by defining \$GTAGS_LANG.
  The default style can be set by definiting \$GTAGS_STYLE.
  Match whole words only by default if \$GTAGS_WORD is 1.
  Match invocations and definitions by default if \$GTAGS_INVOCATIONS is 1.
  By default restrict matches to $GTAGS_RESTRICT, if set.
  Default basedir can be overridden by setting \$GTAGS_BASEDIR.
  Default proxy can be overridden by setting \$GTAGS_PROXY.
  It is possible to disable colors by default by setting \$GTAGS_COLOR to 0.
EOF
}

# Check for sanity of default corpus. If invalid, just ignore it
if [ ! "$GTAGS_CORPUS" = "google3" -a \
       "$GTAGS_CORPUS" = "googleclient_wireless" ]; then
   unset $GTAGS_CORPUS;
fi;
corpus=${GTAGS_CORPUS:-"google3"} # Set default corpus.  default: "google3"

# Check for sanity of default lang. If invalid, just ignore it
if [ ! "$GTAGS_LANG" = "c++" -a \
     ! "$GTAGS_LANG" = "java" -a \
     ! "$GTAGS_LANG" = "python" ]; then
   unset $GTAGS_LANG;
fi;
lang=${GTAGS_LANG:-"c++"} # Set default formatting lang, c++ if unspecified

# match full word
if [ "$GTAGS_WORD" = "1" ]; then
  word=1;
else
  word=0;
fi;

# search for invocations
if [ "$GTAGS_INVOCATIONS" = "1" ]; then
  invocations=1;
else
  invocations=0;
fi;

# Check for sanity of default style. If invalid, just ignore it
if [ ! "$GTAGS_STYLE" = "plain" -a \
     ! "$GTAGS_STYLE" = "less" -a \
     ! "$GTAGS_STYLE" = "vi" ]; then
   unset $GTAGS_STYLE;
fi;
style=${GTAGS_STYLE-"less"} # Set default formatting style, less if unspecified

# Restrict to o given sub-directory
if [ "$GTAGS_RESTRICT" != "" ]; then
  restrictcmd="grep '[ /]$GTAGS_RESTRICT/'";
else
  restrictcmd="cat";
fi;

# Base directory for returned entries
if [ "$GTAGS_BASEDIR" != "" ]; then
  basedir="$GTAGS_BASEDIR";
fi;

# Set color only if this is a terminal window and it was not disabled by the
# user configuration
if [ "$GTAGS_COLOR" = "0" ]; then
  color=0;
else
  if perl -e 'exit -t STDOUT'; then
    color=0;
  else
    color=1;
  fi;
fi;

opts=`getopt -o L:wiplvsr: \
      --long word,invocations,plain,less,vi,\
corpus:,lang:,restrict:,basedir:,color,nocolor,proxy:,status \
      -n 'gtags.sh' -- "$@"`

eval set -- "$opts"
while true ; do
  case "$1" in
    --corpus)
      corpus=$2; shift 2 ;;
    -L|--lang)
      # Validate the argument of --lang
      if [ ! "$2" = "c++" -a \
          ! "$2" = "java" -a \
          ! "$2" = "python" -a ]; then
        echo "$0: invalid language: $2";
        exit 1
      fi;
      lang=$2; shift 2 ;;
    --proxy)		proxy=$2; shift 2 ;;
    -w|--word)		word=1; shift ;;
    -i|--invocations)	invocations=1; shift ;;
    # -p, -l, -v, and -s are mutually exclusive.
    -p|--plain)		style="plain"; shift;;
    -l|--less)		style="less"; shift;;
    -v|--vi)		style="vi"; shift;;
    --status)           checkstatus=1; shift ;;
    -r|--restrict)
      # Use -r - to force no restrictions
      if [ "$2" = "-" ]; then
        restrictcmd="cat";
      else
        restrictcmd="grep '[ /]$2/'";
      fi;
      shift 2 ;;
    --basedir)		basedir="$2"; shift 2 ;;
    --color)		color=1; shift ;; # color regardless of isatty(stdout)
    --nocolor)		color=0; shift ;; # nocolor regardless of isatty(stdout)
    --)			shift ; break ;;
    *)			echo "Internal error!" ; exit 1 ;;
  esac
done

set_proxy=""
if [ "$proxy" != "" ] ; then
    set_proxy="gtags.connection_manager.proxy='"$proxy"'"
fi

if [ "$*" = "" ]; then
    if [ "$checkstatus" = "1" ]; then
	PYTHONPATH=$(dirname $0):$PYTHONPATH python -c "
import gtags

$set_proxy
result = gtags.check_server_status('"$lang"', $invocations, 'shell')
print 'Server status: ' + result"
	exit 0
    else
	usage
	exit 1
    fi;
fi

if [ "$basedir" != "" -a "`echo "$basedir" | sed 's/.*\(.\)$/\1/'`" != "/" ];
then
    basedir="$basedir/"
fi

# "* file:line" instead of "file:line", so that grep \[ /]dir/ works uniformly
case "$style" in
  plain)
    outfmt="'* $basedir' + result.filename_ + ':' + str(result.lineno_)";;
  vi)
    outfmt="'vi +' + str(result.lineno_) + ' $basedir' + result.filename_";;
 # less)
  *)
    outfmt="'less +' + str(result.lineno_) + ' $basedir' + result.filename_";;
    # In case of a mistake, use less formatting
esac;

if [ "$color" = "1" ]; then   # ansi escape sequences for bright colors
  red="'\033[1;31m'"
  green="'\033[1;32m'"
  yellow="'\033[1;33m'"
  blue="'\033[1;34m'"
  magenta="'\033[1;35m'"
  cyan="'\033[1;36m'"
  white="'\033[1;37m'"
  black="'\033[0m'"
else
  red="''"
  green="''"
  yellow="''"
  blue="''"
  magenta="''"
  cyan="''"
  white="''"
  black="''"
fi

columns=${COLUMNS:-80}  # columns=$COLUMNS if defined; 80 otherwise

for tag; do
call=""
if [ "$word" = "1" ]; then 
    call="find_matching_tags_exact"; #exact match
else
    call="find_matching_tags"; #regexp match
fi


# talk to the gtags server

# print output as
#   line1: file_and_lineno_as_per_outfmt_defined_above
#   line2: snippet
#   line3: blank

# However, to retain the ability to grep over such outputs, we pad each
# line with $COLUMNS-length(line) whitespaces, and emit all three output
# lines as one long line.  Thus I coded my first darksome tidbit of python.


PYTHONPATH=$(dirname $0):$PYTHONPATH python -c "
import gtags
$set_proxy
try:
  results = gtags."$call"('"$lang"', '"$tag"', "$invocations", 0, 'shell')
except gtags.ErrorMessageFromServer, msg:
  print $red + str(msg) + $black
  sys.exit(-1)
for result in results:
  if result.filename_[0:2] == './':      # strip ./ which shows up in -i mode
    result.filename_ = result.filename_[2:]
  line1 = $outfmt
  pad1 = ' ' * ($columns - (len(line1) % $columns))
  line2 = result.snippet_
  pad2 = ' ' * ($columns - (len(line2) % $columns))
  print line1 + pad1 + $blue + line2 + $black + pad2 + (' ' * 80)"  \
| eval $restrictcmd   # restrict matches to a directory if specified; else cat
done
