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

import {ReactionsViewModel} from "../viewmodels/reactions_viewmodel";

export const reactionsComponent = {
    props: {
        self: ReactionsViewModel
    },
    template: `
      <div class="form-group row">
        <div class="col-sm-1">Zmojis:</div>
        <div class="col-sm-5">
          <button v-for="zmoji in self.getZmojisForReactions()"
                  class="btn btn-sm btn-outline-dark"
                  @click="zmoji.click()">
            {{ zmoji.text }}
          </button>
        </div>
      </div>

      <div class="form-group row">
        <div class="col-sm-1">Add New:</div>
        <div class="col-sm-5">
          <div class="input-group input-group-sm">
            <input type="text" class="form-control" placeholder="new reaction/hashtag"
                   v-model="self.reactionText">
            <span class="input-group-btn">
            <button class="btn btn-success"
                    @click="self.submitNewReaction()"
                    :disabled="!self.okToSubmitNewReaction">
              Add
            </button>
            </span>
          </div>
        </div>
      </div>
    `
}

