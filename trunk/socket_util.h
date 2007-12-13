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
// Convencience functions for socket-related activities.
// Note: Currently, they are only provided for testing usage.

#ifndef TOOLS_TAGS_SOCKET_UTIL_H__
#define TOOLS_TAGS_SOCKET_UTIL_H__

namespace gtags {

// Finds an available port that is not currently listening or connecting.
// Consecutive calls will not necessarily return different ports.
int FindAvailablePort();

}  // namespace gtags

#endif  // TOOLS_TAGS_SOCKET_UTIL_H__
