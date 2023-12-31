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

export const zgramBodyComponent = {
    props: {
        zg: ZgramViewModel
    },
    mounted() {
        this.zg.maybeUpdateMathJax(this.$el);
    },
    updated() {
        this.zg.maybeUpdateMathJax(this.$el);
    },
    template: `
      <div v-if="zg.weaklyHidden">
        <i>This content has been suppressed by one of your filters.</i>
        <button type="button" class="btn btn-warning btn-sm" @click="zg.overrideWeaklyHidden()">Show</button>
      </div>
      <div v-else v-html="zg.bodyAsHtml" @click="zg.doClick()"
           :class="{zgramHoveringBody: zg.isHovering, zgramUnreadBody: !zg.isHovering && zg.isUnread}">
      </div>
    `
}
