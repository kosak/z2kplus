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

import {SpeechViewmodel} from "../viewmodels/speech_viewmodel";

export const speechDialogComponent = {
    props: {
        self: SpeechViewmodel
    },
    mounted() {
        this.$refs.modal.addEventListener('shown.bs.modal', () => {
            this.self.onShown();
        });
    },
    template: `
      <div ref="modal" id="speechDialog" class="modal modal-lg fade" tabindex="-1">
      <div class="modal-dialog">
        <div class="modal-content">
          <div class="modal-header">
            <h5 class="modal-title">Speech Settings</h5>
            <button tabIndex="-1" type="button" class="btn-close" data-bs-dismiss="modal">
            </button>
          </div>
          <div class="modal-body">
            <div class="container-fluid">
              <i><label>Speech Settings</label></i>
              <select v-model="self.selectedVoice">
                <option v-for="v in self.availableVoices" :value="v">
                  {{ v.name }}
                </option>
              </select>
              <button @click="self.test()">
                Test
              </button>
            </div>
          </div>
        </div>
      </div>
      </div>
    `
};
