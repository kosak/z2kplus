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

export const zgramHeaderComponent = {
    props: {
        zg: ZgramViewModel
    },
    template: `
      <div class="zgramHeader" :class="{zgramUnloggedBody: !zg.isLogged}">
      <!-- Reply button, signature, sender, instance, timestamp -->

      <button class="badge rounded-pill text-bg-light" @click="zg.toggleReplyInteraction()"
              :class="{'fa-bounce': zg.hasHiddenReplyDraft, 'bg-warning': zg.hasHiddenReplyDraft }">
        <i class="fa fa-reply"></i>
      </button>

      <span>&nbsp;</span>

      <span><i>{{ zg.signature }}</i></span>

      (<a :href="zg.makeUriForQueryOnSender()" target="_blank">{{ zg.sender }}</a>)

      <b><a :href="zg.makeUriForQueryOnInstance()" target="_blank">{{ zg.instance }}</a></b>

      [<a :href="zg.makeUriForQueryOnTimestamp()" target="_blank">{{ zg.customerReadableDate }}</a>]

      <button class="badge text-bg-light" title="Show Controls"
              @click="zg.toggleControls()">
        <i class="fa fa-light fa-space-shuttle"></i>
      </button>

      <button class="badge text-bg-light" title="Speak this Zgram"
              @click="zg.doSpeak()">
        <i class="fa fa-light fa-ear-listen"></i>
      </button>

      <template v-if="zg.controlsEnabled">
        <button class="badge text-bg-light" title="Add Filter"
                @click="zg.toggleAddFilterInteraction()">
          <i class="fa fa-light fa-head-side-mask"></i>
        </button>

        <button class="badge text-bg-light" title="Reactions/Hashtags"
                :class="{ 'text-muted': zg.reactionsViewModel.numReactions !== 0 }"
                @click="zg.toggleReactionsInteraction()">
          <i class="fa fa-light fa-hashtag"></i>
        </button>

        <button class="badge text-bg-light" title="Edit Zgram"
                :class="{'fa-bounce': zg.hasHiddenEditDraft, 'bg-warning': zg.hasHiddenEditDraft }"
                :disabled="!zg.canEdit"
                @click="zg.toggleEditZgram()">
          <i class="fa fa-light fa-strikethrough"></i>
        </button>

        <button class="badge badge-pill text-bg-light" title="Q-Decode"
                :disabled="!zg.isQDecodeEnabled" @click="zg.performQDecode()">
          <i class="fa fa-light fa-quora"></i>
        </button>

        <button class="badge badge-pill text-bg-light" title="Show source (3-way toggle)"
                @click="zg.toggleRendering()">
          <i class="fa fa-light fa-transgender-alt"></i>
        </button>

        <button class="badge badge-pill text-bg-light" title="Copy link to zgram"
                @click="zg.copyLinkToZgram()">
          <i class="fa fa-light fa-link"></i>
        </button>

        <button class="badge text-bg-light" title="help"
                data-bind="click: toggleHelpInteraction">
          <i class="fa fa-light fa-question-circle"></i>
        </button>
      </template>
      </div>
    `
}
