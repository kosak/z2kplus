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

import {addFilterComponent} from "./add_filter_component";
import {reactionsComponent} from "./reactions_component";
import {zgramEditorComponent} from "./zgram_editor_component";
import {ZgramViewModel} from "../viewmodels/zgram_viewmodel";

export const zgramFooterComponent = {
    components: {
        addFilterComponent,
        reactionsComponent,
        zgramEditorComponent
    },
    props: {
        zg: ZgramViewModel
    },
    template: `
      <div v-if="zg.plusPlusesViewModel.numPlusPluses !== 0">
      <span v-for="pp in zg.plusPlusesViewModel.getPlusPluses()">
        <small>
        <span class="plusplus-count-button">
          <span class="plusplus-count-button-inner">
            {{ pp.key }}: {{ pp.count }} <i class="fa fa-light"></i>
          </span>
        </span>
        </small>
        &thinsp;
      </span>
      </div>

      <div v-if="zg.reactionsViewModel.numReactions !== 0">
        <div style="line-height: 25%">
          <br>
        </div>
        <span v-for="r in zg.reactionsViewModel.getReactions()">
        <button class="btn btn-sm btn-outline-dark position-relative"
                @click="r.doClick()">
          {{ r.text }}
          <span class="position-absolute top-0 start-100 translate-middle badge rounded-pill text-white"
                :class="{'bg-success': r.includesMe, 'bg-info': !r.includesMe }">
              {{ r.count }}
            </span>
        </button>
        <button class="btn btn-sm btn-outline-info"
                @click="r.doUpvote()">
          <i class="fa-regular fa-thumbs-up"></i>
        </button>
      </span>
      </div>

      <div v-if="zg.hasEdits">
      <input type="range" class="slider" min="0"
             :max="zg.maxVersion"
             v-model="zg.versionSelector">
      </div>

      <div v-if="zg.isAddFilterVisible">
        <hr>
        <add-filter-component :self="zg.addFilterViewModel">
        </add-filter-component>
      </div>

      <div v-if="zg.isReplyVisible">
        <hr>
        <zgram-editor-component :self="zg.replyViewModel">
        </zgram-editor-component>
      </div>

      <div v-if="zg.isEditZgramVisible">
        <hr>
        <zgram-editor-component :self="zg.editZgramViewModel">
        </zgram-editor-component>
      </div>

      <div v-if="zg.isReactionsVisible">
        <hr>
        <reactions-component :self="zg.reactionsViewModel">
        </reactions-component>
      </div>
    `
}
