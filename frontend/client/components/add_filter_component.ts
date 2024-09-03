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

import {AddFilterViewModel} from "../viewmodels/add_filter_viewmodel";

export const addFilterComponent = {
    props: {
        self: AddFilterViewModel
    },
    template: `
      <div>
      <div v-for="item in self.whichChoices" class="form-check form-check-inline">
        <label class="form-check-label">
          <input tabIndex="-1" class="form-check-input" type="radio" v-model="self.which" :value="item.tag">
          {{ item.text }}
        </label>
      </div>
      </div>

      <div>
      <div v-for="item in self.whichStrengths" class="form-check form-check-inline">
        <label class="form-check-label">
          <input tabIndex="-1" class="form-check-input" type="radio" v-model="self.strong" :value="item.tag">
          {{ item.text }}
        </label>
      </div>
      </div>

      <button class="btn btn-success" type="submit"
              @click="self.addFilter()">
      Add
      </button>
    `
}
