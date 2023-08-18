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

import {StreamStatusViewModel} from "../viewmodels/stream_status_viewmodel";

export const streamStatusComponent = {
    props: {
        self: StreamStatusViewModel
    },
    template: `
      <div>
      <span class="sourceSans">
        {{ self.humanReadableEstimateText }}
      </span>
      <button v-for="b in self.buttons" class="btn btn-sm btn-outline-dark" @click="b.doClick()">
        {{ b.text }}
      </button>
      <span class="form-check form-check-inline">
        <input class="form-check-input" type="checkbox" value="" :id="self.uniqueId" v-model="self.wantSpeech">
        <label class="form-check-label" v-bind:for="self.uniqueId">
          Speak these
        </label>
      </span>
      </div>
    `
}
