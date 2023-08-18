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

#pragma once
#include <utility>

namespace kosak::coding {

template<typename R, typename... ARGS>
class Delegate {
public:
  typedef R (*funcp_t)(const void *, ARGS...);

  template<typename F>
  Delegate(const F *f) {
    funcp_ = (R(*)(const void *, ARGS...)) &dispatcher<F>;
    selfp_ = f;
  }

  R operator()(ARGS... args) const {
    return (*funcp_)(selfp_, std::forward<ARGS>(args)...);
  }

private:
  template<typename F>
  static R dispatcher(const void *selfp, ARGS... args) {
    auto *typed_selfp = (F *) selfp;
    return typed_selfp->operator()(std::forward<ARGS>(args)...);
  }

  funcp_t funcp_;
  const void *selfp_;
};
}  // namespace kosak::coding
