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

import {ZgramViewModel} from "../viewmodels/zgram_viewmodel";

export const zgramRefersToComponent = {
    props: {
        zg: ZgramViewModel
    },
    template: `
      <div class="referredToZgram">
      <i class="fa fa-duotone fa-reply-all"></i>

      (<a :href="zg.makeUriForQueryOnSender()">{{ zg.sender }}</a>)

      <b><a :href="zg.makeUriForQueryOnInstance()">{{ zg.instance }}</a></b>

      [<a :href="zg.makeUriForQueryOnTimestamp()">{{ zg.customerReadableDate }}</a>]

      {{ zg.abbreviatedBody }}

      </div>
    `
}
