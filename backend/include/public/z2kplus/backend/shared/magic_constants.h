// Copyright 2023 The Z2K Plus+ Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <chrono>
#include <cstdlib>
#include <regex>
#include <fcntl.h>
#include "kosak/coding/coding.h"

namespace z2kplus::backend::shared {
namespace magicConstants {

constexpr int listenPort = 8001;
constexpr size_t nearMargin = 3;
constexpr size_t numIndexBuilderShards = 4;

constexpr auto purgeInterval = std::chrono::minutes(5);
// ideally, more like once an hour?
constexpr auto reindexingInterval = std::chrono::hours(8);
constexpr auto unloggedLifespan = std::chrono::hours(24 * 7);
constexpr const char *zalexaId = "zalexa";
constexpr const char *zalexaSignature = "Zalexa";

constexpr size_t iteratorChunkSize = 256;

constexpr size_t zgramCacheSize = 500;

constexpr size_t maxPlusPlusKeySize = 256;

extern std::regex plusPlusRegex;

namespace filenames {
const auto standardFlags = O_CREAT | O_EXCL | O_WRONLY;
const auto standardMode = 0644;

constexpr const char *beforeSortingSuffix = ".before_sorting";
constexpr const char *canonicalStrings = "canonical_strings";
constexpr const char *tempFileForTupleCounts = "tuple_counts_temp";
constexpr const char *loggedZgrams = "zgrams_logged";
constexpr const char *unloggedZgrams = "zgrams_unlogged";
constexpr const char *plusPlusEntries = "plusplus_entries";
constexpr const char *minusMinusEntries = "minusminus_entries";
constexpr const char *plusPlusKeys = "plusplus_keys";
constexpr const char *reactionsByZgramId = "reactions_by_zgid";
constexpr const char *reactionsByReaction = "reactions_by_reaction";
constexpr const char *trieEntries = "trie_entries";
constexpr const char *wordInfos = "wordInfos";
constexpr const char *zgramInfos = "zgramInfos";
constexpr const char *zgramRevisions = "zgram_revisions";
constexpr const char *zgramRefersTo = "zgram_refers_to";
constexpr const char *zmojis = "zmojis";
}  // namespace filenames
}  // namespace magicConstants
}  // namespace z2kplus::backend::shared
