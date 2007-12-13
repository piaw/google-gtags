#!/usr/bin/python2.4
#
# Copyright 2007 Google Inc. All Rights Reserved.

"""A sample .py file for local_gentags testing."""

__author__ = "nigdsouza@google.com (Nigel D'souza)"

import sys


def HelloWorld(name):
  """Prints 'Hello World' with the given name."""
  print 'Hello World, %s!' % name

if __name__ == '__main__':
  HelloWorld(sys.argv[1])
