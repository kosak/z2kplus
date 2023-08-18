// Copyright 2023 Corey Kosak
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

#include <sys/time.h>
#include <string>

#include "kosak/coding/coding.h"
#include "kosak/coding/time.h"

namespace kosak::coding {
namespace time {

time_t now() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec;
}

std::pair<time_t, size_t> now2() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return std::make_pair(tv.tv_sec, tv.tv_usec);
}


uint64 nowMicros() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return ((uint64)tv.tv_sec) * 1'000'000 + tv.tv_usec;
}

std::string toString(time_t time) {
  struct tm tm;
  localtime_r(&time, &tm);
  char buffer[256];
  strftime(buffer, sizeof(buffer), "%a, %d %b %y %T %z", &tm);
  return std::string(buffer);
}

}  // namespace time
}  // namespace kosak::coding
