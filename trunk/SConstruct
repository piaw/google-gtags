# Copyright 2007 Google Inc. All Rights Reserved.
# Author: nigdsouza@google.com (Nigel D'souza)

"""GTags SConstruct file.

There are 4 main types of targets: libraries, binaries, tests, and run-tests.
Targets for each type are collected in separate lists.  This allows easily
building types of targets by using the type as a target.
  Examples:
    'scons libs' will build all the libraries.
    'scons tests' will build all the tests.

Each type is also contained within it's own environment due to attribute
differences (such as pathing, flags, and builders).

All targets are aliased with their name so that, regardless of type, they can be
built with 'scons <target name>'.  This is easier to remember than the default
name, which is the name of the target's output file.

All tests also have a run target defined.  This is aliased with the name
'run:<test name>' so that they can be run with 'scons run:<test_name>'.
"""

import os

CC_TEST_FLAGS = ('-DTEST_TMP_DIR=\\"/tmp/gtags.test\\"'
                 ' -DTEST_SRC_DIR=\\"%s\\"'% os.getcwd())

# Define library environment and build helper.
# The library environment also defines the CC_TEST_FLAGS for the test libraries
libs = []
lib_env = Environment()
def library(name, srcs):
  l = lib_env.Library(name, srcs)
  # Alias the library with it's name otherwise by default it gets the filename
  # of the generated library (e.g. 'filename.cc' gets 'libfilename.a')
  lib_env.Alias(name, l)
  libs.append(l)
  lib_env.Alias('libs', libs)


# Define binary envrionment and build helper.
bins = []
bin_env = Environment(LIBPATH = '.')
def binary(name, srcs, deps=[]):
  b = bin_env.Program(
      name, srcs,
      # Link the dependents twice so we don't have to worry about the order of
      # linked libraries.
      LIBS = deps + deps)
  bins.append(b)
  bin_env.Default(b)
  bin_env.Alias('bins', bins)


# Define test environment and build helper.
tests = []
TEST_LIBS = [ 'boost_unit_test_framework', 'tagsoptionparser' ]
test_env = Environment(
    LIBPATH = '.',
    CCFLAGS = CC_TEST_FLAGS,
    CPPPATH = '../..')
def test(name, srcs, deps=[]):
  t = test_env.Program(
      name, srcs,
      # The tests seem to be considerable tougher to link, so we'll cheat and
      # link the dependents thrice so we don't have to worry about the order of
      # linked libraries.
      LIBS = TEST_LIBS + deps + deps + deps)
  tests.append(t)
  test_env.Alias('tests', tests)

  # Create a run target for this test.
  rt = run_test_env.RunTest('run:' + name, t)
  run_tests.append(rt)
  run_test_env.AlwaysBuild(rt)
  run_test_env.Alias('runtests', run_tests)


# Define run test environment and builder.
def run_test(target, source, env):
  # Create the output directory.
  test_dir_name = 'test_out'
  if not os.path.exists(test_dir_name):
    os.mkdir(test_dir_name)

  # Create the temp directory.
  tmp_dir_name = '/tmp/gtags.test'
  if not os.path.exists(tmp_dir_name):
    os.mkdir(tmp_dir_name)

  # Run the test, storing the test output to a file.
  output_file = os.path.join(test_dir_name, source[0].name + '.out')
  cmd = str(source[0].abspath) + " >" + output_file + " 2>&1"

  # Display the test result.
  if os.system(cmd) == 0:
    print '    %-40s %s' % (source[0].name, 'PASSED')
  else:
    print '    %-40s %s' % (source[0].name, 'FAILED')
    print '      See test output in file: %s' % os.path.abspath(output_file)

run_tests = []
RUN_TEST_BUILDER = Builder(action = run_test)
run_test_env = Environment(BUILDERS = {'RunTest' : RUN_TEST_BUILDER })


# Libraries
# ==========================================================

library(name = 'datasource',
        srcs = 'datasource.cc')

library(name = 'filename',
        srcs = 'filename.cc')

library(name = 'filewatcher',
        srcs = 'filewatcher.cc')

library(name = 'filewatcherrequesthandler',
        srcs = 'filewatcherrequesthandler.cc')

library(name = 'indexagent',
        srcs = 'indexagent.cc')

library(name = 'mixer',
        srcs = 'gtagsmixer.cc')

library(name = 'mixerrequesthandler',
        srcs = 'mixerrequesthandler.cc')

library(name = 'mock_socket',
        srcs = 'mock_socket.cc')

library(name = 'pollable',
        srcs = 'pollable.cc')

library(name = 'pollserver',
        srcs = 'pollserver.cc')

library(name = 'settings',
        srcs = 'settings.cc')

library(name = 'sexpression',
        srcs = 'sexpression.cc')

library(name = 'sexpression_util',
        srcs = 'sexpression_util.cc')

library(name = 'socket',
        srcs = 'socket.cc')

library(name = 'socket_filewatcher_service',
        srcs = 'socket_filewatcher_service.cc')

library(name = 'socket_mixer_service',
        srcs = 'socket_mixer_service.cc')

library(name = 'socket_server',
        srcs = 'socket_server.cc')

library(name = 'socket_util',
        srcs = 'socket_util.cc')

library(name = 'socket_tags_service',
        srcs = 'socket_tags_service.cc')

library(name = 'socket_version_service',
        srcs = 'socket_version_service.cc')

library(name = 'strutil',
        srcs = 'strutil.cc')

library(name = 'symboltable',
        srcs = 'symboltable.cc')

library(name = 'tagsoptionparser',
        srcs = 'tagsoptionparser.cc')

library(name = 'tagsprofiler',
        srcs = 'tagsprofiler.cc')

library(name = 'tagsrequesthandler',
        srcs = 'tagsrequesthandler.cc')

library(name = 'tagstable',
        srcs = 'tagstable.cc')

# Applications
# ==========================================================

binary(name = 'gtags',
       srcs = 'gtags.cc',
       deps = [ 'filename',
                'socket_server',
                'symboltable',
                'sexpression',
                'strutil',
                'tagsoptionparser',
                'tagsprofiler',
                'tagsrequesthandler',
                'tagstable' ])

binary(name = 'gtagsmixer',
       srcs = 'gtagsmixermain.cc',
       deps = [ 'datasource',
                'filename',
                'filewatcher',
                'filewatcherrequesthandler',
                'indexagent',
                'mixer',
                'mixerrequesthandler',
                'pollable',
                'pollserver',
                'settings',
                'sexpression',
                'sexpression_util',
                'socket',
                'socket_filewatcher_service',
                'socket_mixer_service',
                'socket_version_service',
                'socket_tags_service',
                'strutil',
                'symboltable',
                'tagsoptionparser',
                'tagsrequesthandler',
                'tagstable',
                'pthread' ])

binary(name = 'gtagswatcher',
       srcs = 'gtagswatchermain.cc',
       deps = [ 'pollable',
                'pollserver',
                'sexpression',
                'socket',
                'socket_util',
                'socket_filewatcher_service',
                'strutil',
                'tagsoptionparser' ])

# Tests
# ==========================================================

test(name = 'callback_test',
     srcs = 'callback_test.cc'),

test(name = 'filename_test',
     srcs = 'filename_test.cc',
     deps = [ 'filename',
              'symboltable' ])

test(name = 'filewatcher_test',
     srcs = 'filewatcher_test.cc',
     deps = [ 'filewatcher' ])

test(name = 'filewatcherrequesthandler_test',
     srcs = 'filewatcherrequesthandler_test.cc',
     deps = [ 'filewatcherrequesthandler',
              'filename',
              'filewatcher',
              'strutil',
              'sexpression',
              'symboltable',
              'tagsrequesthandler',
              'tagstable' ])

test(name = 'indexagent_test',
     srcs = 'indexagent_test.cc',
     deps = [ 'indexagent',
              'filename',
              'sexpression',
              'strutil',
              'symboltable',
              'tagsrequesthandler',
              'tagstable' ])

test(name = 'mixer_test',
     srcs = 'gtagsmixer_test.cc',
     deps = [ 'mixer',
              'sexpression',
              'sexpression_util',
              'strutil' ])

test(name = 'mixerrequesthandler_test',
     srcs = 'mixerrequesthandler_test.cc',
     deps = [ 'mixerrequesthandler',
              'datasource',
              'filename',
              'mixer',
              'pollable',
              'pollserver',
              'socket',
              'socket_tags_service',
              'settings',
              'sexpression',
              'sexpression_util',
              'strutil',
              'symboltable',
              'tagstable',
              'tagsrequesthandler',
              'pollable' ])

test(name = 'mutex_test',
     srcs = 'mutex_test.cc')

test(name = 'pcqueue_test',
     srcs = 'pcqueue_test.cc')

test(name = 'pollable_test',
     srcs = 'pollable_test.cc',
     deps = [ 'pollable',
              'pollserver' ])

test(name = 'pollserver_test',
     srcs = 'pollserver_test.cc',
     deps = [ 'pollserver',
              'pollable' ])

test(name = 'semaphore_test',
     srcs = 'semaphore_test.cc')

test(name = 'settings_test',
     srcs = 'settings_test.cc',
     deps = [ 'settings',
              'datasource',
              'filename',
              'mixer',
              'pollable',
              'pollserver',
              'sexpression',
              'sexpression_util',
              'socket',
              'socket_tags_service',
              'strutil',
              'symboltable',
              'tagstable',
              'tagsrequesthandler' ])

test(name = 'sexpression_test',
     srcs = 'sexpression_test.cc',
     deps = [ 'sexpression',
              'datasource',
              'socket_tags_service',
              'strutil' ])

test(name = 'sexpression_util_test',
     srcs = 'sexpression_util_test.cc',
     deps = [ 'sexpression',
              'sexpression_util',
              'strutil' ])

test(name = 'socket_test',
     srcs = 'socket_test.cc',
     deps = [ 'socket',
              'mock_socket',
              'pollable',
              'pollserver',
              'socket_util' ])

test(name = 'socket_filewatcher_service_test',
     srcs = 'socket_filewatcher_service_test.cc',
     deps = [ 'socket_filewatcher_service',
              'filewatcherrequesthandler',
              'mock_socket',
              'pollable',
              'pollserver',
              'sexpression',
              'socket',
              'socket_util',
              'strutil' ])

test(name = 'socket_mixer_service_test',
     srcs = 'socket_mixer_service_test.cc',
     deps = [ 'socket_mixer_service',
              'datasource',
              'filename',
              'mixer',
              'mixerrequesthandler',
              'mock_socket',
              'pollable',
              'pollserver',
              'settings',
              'sexpression',
              'sexpression_util',
              'socket',
              'socket_tags_service',
              'socket_util',
              'strutil',
              'symboltable',
              'tagsrequesthandler',
              'tagstable' ])

test(name = 'socket_version_service_test',
     srcs = 'socket_version_service_test.cc',
     deps = [ 'socket_version_service',
              'mock_socket',
              'pollable',
              'pollserver',
              'socket',
              'socket_util',
              'strutil' ])

test(name = 'stl_util_test',
     srcs = 'stl_util_test.cc')

test(name = 'strutil_test',
     srcs = 'strutil_test.cc',
     deps = [ 'strutil' ])

test(name = 'symboltable_test',
     srcs = 'symboltable_test.cc',
     deps = [ 'symboltable' ])

test(name = 'tagsrequesthandler_test',
     srcs = 'tagsrequesthandler_test.cc',
     deps = [ 'tagsrequesthandler',
              'filename',
              'sexpression',
              'strutil',
              'symboltable',
              'tagstable' ])

test(name = 'tagstable_test',
     srcs = 'tagstable_test.cc',
     deps = [ 'tagstable',
              'filename',
              'sexpression',
              'strutil',
              'symboltable' ])

test(name = 'thread_test',
     srcs = 'thread_test.cc')
