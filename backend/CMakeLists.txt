cmake_minimum_required(VERSION 3.14)
project(backend)
# set(CMAKE_VERBOSE_MAKEFILE true)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
set(CMAKE_COLOR_DIAGNOSTICS OFF)
# set(CMAKE_INCLUDE_CURRENT_DIR on)
# set(CMAKE_VERBOSE_MAKEFILE on)

set(COMMON_FILES
        include/public/z2kplus/backend/communicator/channel.h
        include/public/z2kplus/backend/communicator/communicator.h
        include/public/z2kplus/backend/communicator/message_buffer.h
        include/public/z2kplus/backend/communicator/robustifier.h
        include/public/z2kplus/backend/communicator/session.h
        include/public/z2kplus/backend/coordinator/coordinator.h
        include/public/z2kplus/backend/coordinator/subscription.h
        include/public/z2kplus/backend/factories/log_parser.h
        include/public/z2kplus/backend/files/keys.h
        include/public/z2kplus/backend/files/path_master.h
        include/public/z2kplus/backend/queryparsing/parser.h
        include/public/z2kplus/backend/queryparsing/util.h
        include/public/z2kplus/backend/reverse_index/fields.h
        include/public/z2kplus/backend/reverse_index/builder/canonical_string_processor.h
        include/public/z2kplus/backend/reverse_index/builder/common.h
        include/public/z2kplus/backend/reverse_index/builder/index_builder.h
        include/public/z2kplus/backend/reverse_index/builder/inflator.h
        include/public/z2kplus/backend/reverse_index/builder/log_analyzer.h
        include/public/z2kplus/backend/reverse_index/builder/log_splitter.h
        include/public/z2kplus/backend/reverse_index/builder/metadata_builder.h
        include/public/z2kplus/backend/reverse_index/builder/schemas.h
        include/public/z2kplus/backend/reverse_index/builder/trie_builder.h
        include/public/z2kplus/backend/reverse_index/builder/trie_finalizer.h
        include/public/z2kplus/backend/reverse_index/builder/zgram_digestor.h
        include/public/z2kplus/backend/reverse_index/builder/tuple_iterators/accumulator.h
        include/public/z2kplus/backend/reverse_index/builder/tuple_iterators/counter.h
        include/public/z2kplus/backend/reverse_index/builder/tuple_iterators/emitter.h
        include/public/z2kplus/backend/reverse_index/builder/tuple_iterators/iterator_base.h
        include/public/z2kplus/backend/reverse_index/builder/tuple_iterators/last_keeper.h
        include/public/z2kplus/backend/reverse_index/builder/tuple_iterators/prefix_grabber.h
        include/public/z2kplus/backend/reverse_index/builder/tuple_iterators/row_iterator.h
        include/public/z2kplus/backend/reverse_index/builder/tuple_iterators/running_sum.h
        include/public/z2kplus/backend/reverse_index/builder/tuple_iterators/true_keeper.h
        include/public/z2kplus/backend/reverse_index/builder/tuple_iterators/tuple_counter.h
        include/public/z2kplus/backend/reverse_index/builder/tuple_iterators/tuple_serializer.h
        include/public/z2kplus/backend/reverse_index/builder/tuple_iterators/string_freezer.h
        include/public/z2kplus/backend/reverse_index/builder/tuple_iterators/util.h
        include/public/z2kplus/backend/reverse_index/index/consolidated_index.h
        include/public/z2kplus/backend/reverse_index/index/dynamic_index.h
        include/public/z2kplus/backend/reverse_index/index/frozen_index.h
        include/public/z2kplus/backend/reverse_index/index/zgram_cache.h
        include/public/z2kplus/backend/reverse_index/iterators/word/anchored.h
        include/public/z2kplus/backend/reverse_index/iterators/zgram/and.h
        include/public/z2kplus/backend/reverse_index/iterators/word/any_word.h
        include/public/z2kplus/backend/reverse_index/iterators/iterator_common.h
        include/public/z2kplus/backend/reverse_index/iterators/zgram/metadata/having_reaction.h
        include/public/z2kplus/backend/reverse_index/iterators/boundary/near.h
        include/public/z2kplus/backend/reverse_index/iterators/zgram/not.h
        include/public/z2kplus/backend/reverse_index/iterators/zgram/or.h
        include/public/z2kplus/backend/reverse_index/iterators/word/pattern.h
        include/public/z2kplus/backend/reverse_index/iterators/zgram/popornot.h
        include/public/z2kplus/backend/reverse_index/iterators/boundary/word_adaptor.h
        include/public/z2kplus/backend/reverse_index/iterators/zgram/zgramid.h
        include/public/z2kplus/backend/reverse_index/metadata/dynamic_metadata.h
        include/public/z2kplus/backend/reverse_index/metadata/frozen_metadata.h
        include/public/z2kplus/backend/reverse_index/trie/dynamic_node.h
        include/public/z2kplus/backend/reverse_index/trie/dynamic_trie.h
        include/public/z2kplus/backend/reverse_index/trie/frozen_node.h
        include/public/z2kplus/backend/reverse_index/trie/frozen_trie.h
        include/public/z2kplus/backend/reverse_index/trie/traversal.h
        include/public/z2kplus/backend/reverse_index/types.h
        include/public/z2kplus/backend/server/server.h
        include/public/z2kplus/backend/shared/plusplus_scanner.h
        include/public/z2kplus/backend/shared/profile.h
        include/public/z2kplus/backend/shared/zephyrgram.h
        include/public/z2kplus/backend/shared/protocol/misc.h
        include/public/z2kplus/backend/shared/protocol/control/crequest.h
        include/public/z2kplus/backend/shared/protocol/control/cresponse.h
        include/public/z2kplus/backend/shared/protocol/message/drequest.h
        include/public/z2kplus/backend/shared/protocol/message/dresponse.h
        include/public/z2kplus/backend/shared/magic_constants.h
        include/public/z2kplus/backend/shared/merge.h
        include/public/z2kplus/backend/shared/logging_policy.h
        include/public/z2kplus/backend/shared/util.h
        include/public/z2kplus/backend/util/automaton/automaton.h
        include/public/z2kplus/backend/util/automaton/fuzzy_unicode.h
        include/public/z2kplus/backend/util/blocking_queue.h
        include/public/z2kplus/backend/util/misc.h
        include/public/z2kplus/backend/util/myallocator.h
        include/public/z2kplus/backend/util/myiterator.h
        include/public/z2kplus/backend/util/mysocket.h
        include/public/z2kplus/backend/util/relative.h
        include/public/z2kplus/backend/util/frozen/frozen_map.h
        include/public/z2kplus/backend/util/frozen/frozen_vector.h
        include/public/z2kplus/backend/util/frozen/frozen_set.h
        include/public/z2kplus/backend/util/frozen/frozen_string_pool.h

        src/communicator/channel.cc
        src/communicator/communicator.cc
        src/communicator/message_buffer.cc
        src/communicator/robustifier.cc
        src/communicator/session.cc
        src/coordinator/coordinator.cc
        src/coordinator/subscription.cc
        src/factories/log_parser.cc
        src/files/keys.cc
        src/files/path_master.cc
        src/queryparsing/generated/ZarchiveParserBaseListener.h
        src/queryparsing/generated/ZarchiveLexer.h
        src/queryparsing/generated/ZarchiveParserListener.h
        src/queryparsing/generated/ZarchiveParser.h
        src/queryparsing/generated/ZarchiveParserBaseListener.cpp
        src/queryparsing/generated/ZarchiveLexer.cpp
        src/queryparsing/generated/ZarchiveParserListener.cpp
        src/queryparsing/generated/ZarchiveParser.cpp
        src/queryparsing/util.cc
        src/queryparsing/parser.cc
        src/reverse_index/fields.cc
        src/reverse_index/builder/canonical_string_processor.cc
        src/reverse_index/builder/common.cc
        src/reverse_index/builder/index_builder.cc
        src/reverse_index/builder/inflator.cc
        src/reverse_index/builder/log_analyzer.cc
        src/reverse_index/builder/log_splitter.cc
        src/reverse_index/builder/metadata_builder.cc
        src/reverse_index/builder/schemas.cc
        src/reverse_index/builder/trie_builder.cc
        src/reverse_index/builder/trie_finalizer.cc
        src/reverse_index/builder/zgram_digestor.cc
        src/reverse_index/builder/tuple_iterators/tuple_counter.cc
        src/reverse_index/builder/tuple_iterators/tuple_serializer.cc
        src/reverse_index/index/consolidated_index.cc
        src/reverse_index/index/dynamic_index.cc
        src/reverse_index/index/frozen_index.cc
        src/reverse_index/index/zgram_cache.cc
        src/reverse_index/iterators/word/anchored.cc
        src/reverse_index/iterators/zgram/and.cc
        src/reverse_index/iterators/word/any_word.cc
        src/reverse_index/iterators/iterator_common.cc
        src/reverse_index/iterators/zgram/metadata/having_reaction.cc
        src/reverse_index/iterators/boundary/near.cc
        src/reverse_index/iterators/zgram/not.cc
        src/reverse_index/iterators/zgram/or.cc
        src/reverse_index/iterators/word/pattern.cc
        src/reverse_index/iterators/zgram/popornot.cc
        src/reverse_index/iterators/boundary/word_adaptor.cc
        src/reverse_index/iterators/zgram/zgramid.cc
        src/reverse_index/metadata/dynamic_metadata.cc
        src/reverse_index/metadata/frozen_metadata.cc
        src/reverse_index/trie/dynamic_node.cc
        src/reverse_index/trie/dynamic_trie.cc
        src/reverse_index/trie/frozen_node.cc
        src/reverse_index/trie/frozen_trie.cc
        src/reverse_index/trie/traversal.cc
        src/reverse_index/types.cc
        src/server/server.cc
        src/shared/magic_constants.cc
        src/shared/logging_policy.cc
        src/shared/plusplus_scanner.cc
        src/shared/profile.cc
        src/shared/zephyrgram.cc
        src/shared/protocol/misc.cc
        src/shared/protocol/control/crequest.cc
        src/shared/protocol/control/cresponse.cc
        src/shared/protocol/message/drequest.cc
        src/shared/protocol/message/dresponse.cc
        src/shared/util.cc
        src/util/automaton/automaton.cc
        src/util/automaton/fuzzy_unicode.cc
        src/util/blocking_queue.cc
        src/util/misc.cc
        src/util/myallocator.cc
        src/util/mysocket.cc
        src/util/frozen/frozen_string_pool.cc
        )

set(TEST_FILES
        test/include/public/z2kplus/backend/test/util/client_endpoint.h
        test/include/public/z2kplus/backend/test/util/fake_frontend.h
        test/include/public/z2kplus/backend/test/util/test_util.h

        test/test_coordinator.cc
        test/test_dfa.cc
        test/test_index_construction.cc
        test/test_main.cc
        test/test_misc.cc
        test/test_metadata.cc
        test/test_paging.cc
        test/test_parsing.cc
        test/test_plusplus.cc
        test/test_reverse_index.cc
        test/test_server.cc
        test/test_tuple_iterators.cc
        test/util/client_endpoint.cc
        test/util/fake_frontend.cc
        test/util/test_util.cc
        test/test_filters.cc
)

add_executable(test_backend ${COMMON_FILES} ${TEST_FILES})
add_executable(backend ${COMMON_FILES} src/backend_main.cc)
add_executable(compactor ${COMMON_FILES} src/compactor_main.cc)

target_include_directories(backend PUBLIC
        "/usr/local/include/antlr4-runtime" "include/public")
target_include_directories(compactor PUBLIC
        "/usr/local/include/antlr4-runtime" "include/public")
target_include_directories(test_backend PUBLIC
        "/usr/local/include/antlr4-runtime" "include/public" "test/include/public")

if(CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_definitions(test_backend PUBLIC DEBUGGING=1)
  target_compile_definitions(backend PUBLIC DEBUGGING=1)
  target_compile_definitions(compactor PUBLIC DEBUGGING=1)
endif()

add_subdirectory("third_party/coding" "libcoding")
add_subdirectory("third_party/catch" "libcatch")

target_link_libraries(test_backend PUBLIC coding catch antlr4-runtime pthread)
target_link_libraries(backend PUBLIC coding antlr4-runtime pthread)
target_link_libraries(compactor PUBLIC coding antlr4-runtime pthread)
