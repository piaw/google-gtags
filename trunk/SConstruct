# Target for gtags executable
env = Environment()
objects=env.Object(['filename.cc',
                    'sexpression.cc',
                    'symboltable.cc',
                    'tagsrequesthandler.cc',
                    'tagstable.cc',
                    'socket_server.cc',
                    'tagsprofiler.cc',
                    'tagsoptionparser.cc',
                    'strutil.cc'])

env.Program("gtags", ["gtags.cc", objects])
Default("gtags")

# Target for unit tests
import glob
import os

test_env = Environment(LIBS='boost_unit_test_framework',
                       CCFLAGS='-DTEST_SRC_DIR=\\"%s\\"'% os.getcwd())

test_list = []
for test in glob.glob("*_unittest.cc"):
  t = test_env.Program([test, objects])
  test_list.append(t)

test_env.Alias('tests', test_list)

# Target for running test
def run_test(target, source, env):
  test_dir_name = "test_out"
  if not os.path.exists(test_dir_name):
    os.mkdir(test_dir_name)

  output_file = os.path.join(test_dir_name, str(target[0].name))
  cmd = str(source[0].abspath) + " >" + output_file + " 2>&1"

  if os.system(cmd) == 0:
    print target[0].name + " PASSED"
  else:
    print target[0].name + " FAILED"
    print "See test output in file: " + str(target[0].abspath)

builder = Builder(action = run_test)
run_test_env = Environment(BUILDERS = {'RunTest' : builder})

run_test_list = []
for test in test_list:
  t = run_test_env.RunTest(test[0].name + ".out", test)
  run_test_list.append(t)
  AlwaysBuild(t)

run_test_env.Alias('runtests', run_test_list)
