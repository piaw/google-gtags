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

#include "gtagsunit.h"
#include "settings.h"

#include "datasource.h"
#include "tagsoptionparser.h"

namespace {

const char* kCorpus1 = "corpus1";
const char* kCorpus2 = "corpus2";
const char* kMixerTestConfigFile = "/mixer_test_socket_config";
const int kExpectedLanguages = 3;
const int kExpectedSources = 1;

TEST(SettingsTest, Load) {
  Settings::Load(TEST_DATA_DIR + kMixerTestConfigFile);
  const DataSourceMap& sources = Settings::instance()->sources();

  // Should be 2 corpuses
  EXPECT_EQ(2, sources.size());
  DataSourceMap::const_iterator corpus1_source = sources.find(kCorpus1);
  EXPECT_TRUE(corpus1_source != sources.end());
  DataSourceMap::const_iterator corpus2_source = sources.find(kCorpus2);
  EXPECT_TRUE(corpus2_source != sources.end());

  // Should be kExpectedLanguages in each corpus.
  EXPECT_EQ(kExpectedLanguages, corpus1_source->second.size());
  EXPECT_EQ(kExpectedLanguages, corpus2_source->second.size());

  LanguageMap::const_iterator i = corpus1_source->second.find("c++");
  // Should be kExpectedSources for both definition and callgraph for c++.
  EXPECT_EQ(kExpectedSources, i->second.first->size());
  EXPECT_EQ(kExpectedSources, i->second.second->size());

  for (DataSourceMap::const_iterator it = sources.begin();
       it != sources.end(); ++it) {
    for (LanguageMap::const_iterator it2 = it->second.begin();
        it2 != it->second.end(); it2++) {
      if (it2->second.first)
        delete it2->second.first;
      if (it2->second.second)
        delete it2->second.second;
    }
  }

  Settings::Free();
}

}  // namespace
