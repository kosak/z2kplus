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

#include "z2kplus/backend/shared/util.h"

namespace z2kplus::backend::shared {
namespace {
struct MetadataRecordVisitor {
  void operator()(const zgMetadata::Reaction &r) {
    zgramId_ = r.zgramId();
  }
  void operator()(const zgMetadata::ZgramRevision &r) {
    zgramId_ = r.zgramId();
  }
  void operator()(const userMetadata::Zmojis &r) {
    userId_ = &r.userId();
  }

  std::optional<ZgramId> zgramId_;
  const std::string *userId_ = nullptr;
};

}  // namespace
std::optional<ZgramId> getZgramId(const MetadataRecord &record) {
  MetadataRecordVisitor v;
  std::visit(v, record.payload());
  return v.zgramId_;
}

const std::string *getUserId(const MetadataRecord &record) {
  MetadataRecordVisitor v;
  std::visit(v, record.payload());
  return v.userId_;
}
}  // namespace z2kplus::backend::shared
