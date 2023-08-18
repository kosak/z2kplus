# Copyright 2023 Corey Kosak
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.12)
project(coding)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_COLOR_DIAGNOSTICS OFF)

add_library(coding
        include/public/kosak/coding/bits.h
        include/public/kosak/coding/cancellation.h
        include/public/kosak/coding/coding.h
        include/public/kosak/coding/comparers.h
        include/public/kosak/coding/containers/slice.h
        include/public/kosak/coding/text/conversions.h
        include/public/kosak/coding/delegate.h
        include/public/kosak/coding/dumping.h
        include/public/kosak/coding/failures.h
        include/public/kosak/coding/farmhash.h
        include/public/kosak/coding/hashtable.h
        include/public/kosak/coding/map_utils.h
        include/public/kosak/coding/merger.h
        include/public/kosak/coding/misc.h
        include/public/kosak/coding/myjson.h
        include/public/kosak/coding/mystream.h
        include/public/kosak/coding/priority_queue.h
        include/public/kosak/coding/range_map.h
        include/public/kosak/coding/recycler.h
        include/public/kosak/coding/ref.h
        include/public/kosak/coding/strongint.h
        include/public/kosak/coding/time.h
        include/public/kosak/coding/unix.h
        include/public/kosak/coding/memory/arena.h
        include/public/kosak/coding/memory/buffered_writer.h
        include/public/kosak/coding/memory/cacheline.h
        include/public/kosak/coding/memory/mapped_file.h
        include/public/kosak/coding/memory/maybe_inlined_buffer.h
        include/public/kosak/coding/memory/memory_tracker.h
        include/public/kosak/coding/sorting/sort_manager.h
        include/public/kosak/coding/text/misc.h
        src/cancellation.cc
        src/coding.cc
        src/text/conversions.cc
        src/dumping.cc
        src/failures.cc
        src/farmhash.cc
        src/hashtable.cc
        src/misc.cc
        src/myjson.cc
        src/mystream.cc
        src/priority_queue.cc
        src/ref.cc
        src/time.cc
        src/unix.cc
        src/memory/arena.cc
        src/memory/buffered_writer.cc
        src/memory/mapped_file.cc
        src/memory/memory_tracker.cc
        src/sorting/sort_manager.cc
        src/text/misc.cc
        )

# mavx2 to make sure that buildint_popcount uses a bitcount processor instruction
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
target_compile_options(coding PUBLIC "-mavx2")

target_include_directories(coding PUBLIC include/public)
