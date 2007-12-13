#!/usr/bin/python2.2
#
# Copyright 2005 Google Inc. All Rights Reserved.
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

"""
Data for gtags port information.

"""

__author__ = 'piaw@google.com (Piaw Na)'


gtags_server_dict = { 'cpp.tags.gz' : ('cpp', 2223),
                      'java.tags.gz' : ('java', 2224),
                      'python.tags.gz' : ('python', 2225),
                      'cpp.callers.tags.gz' : ('cpp_callgraph', 2233),
                      'java.callers.tags.gz' : ('java_callgraph', 2234),
                      'python.callers.tags.gz' : ('python_callgraph', 2235) }
