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
// This is the interface

#ifndef TOOLS_TAGS_MIXER_SERVICE_TEST_H__
#define TOOLS_TAGS_MIXER_SERVICE_TEST_H__

#include "gtagsunit.h"

namespace gtags {

class MixerServiceProvider;

void RunServiceTest(MixerServiceProvider *mixer_provider, int port);

}  // namespace gtags

#endif  // TOOLS_TAGS_MIXER_SERVICE_TEST_H__
