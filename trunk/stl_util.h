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
// Author: nigdsouza@google.com (Nigel D'souza)
//
// Convenience functions for working with STL objects.

#ifndef TOOLS_TAGS_STL_UTIL_H__
#define TOOLS_TAGS_STL_UTIL_H__

template<class ForwardIterator>
void STLDeleteContainerElements(
    ForwardIterator begin, ForwardIterator end) {
  while (begin != end) {
    ForwardIterator temp = begin;
    // Always delete after advancing to avoid invalidating the iterator
    ++begin;
    delete *temp;
  }
}

template<class T>
void STLDeleteElementContainer(T *container) {
  if (!container) return;
  STLDeleteContainerElements(container->begin(), container->end());
  container->clear();
}

// Similar to STLDeleteContainerElements but uses delete[] instead of delete
// since the iterator is of a container of arrays.
template<class ForwardIterator>
void STLDeleteContainerArrays(
    ForwardIterator begin, ForwardIterator end) {
  while (begin != end) {
    ForwardIterator temp = begin;
    // Always delete after advancing to avoid invalidating the iterator
    ++begin;
    delete[] *temp;
  }
}

template<class T>
void STLDeleteArrayContainer(T *container) {
  if (!container) return;
  STLDeleteContainerArrays(container->begin(), container->end());
  container->clear();
}

#endif  // TOOLS_TAGS_STL_UTIL_H__
