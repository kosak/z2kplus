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

import {ZmojisEditorViewModel} from "../viewmodels/zmojis_editor_viewmodel";

export const zmojisEditorDialogComponent = {
    props: {
        self: ZmojisEditorViewModel
    },
    mounted() {
        this.$refs.modal.addEventListener('shown.bs.modal', () => {
            this.$refs.zmojis.focus()
        });
    },
    template: `
      <div ref="modal" id="zmojisEditorDialog" class="modal modal-lg fade" tabindex="-1">
      <div class="modal-dialog">
        <div class="modal-content">
          <div class="modal-header">
            <h5 class="modal-title">Manage Zmojis</h5>
            <button tabIndex="-1" type="button" class="btn-close" data-bs-dismiss="modal">
            </button>
          </div>
          <div class="modal-body">
            <div class="container-fluid">
              <!-- query field -->
              <div class="row">
                <label for="zmojiText" class="col-sm-1 col-form-label">Zmojis</label>
                <div class="col-sm-11">
                  <textarea ref="zmojis" id="zmojiText" tabindex="1" class="form-control form-control-sm" rows="5"
                            placeholder="Comma-separated zmojis"
                            v-model="self.editingZmojis">
                  </textarea>
                </div>
              </div>
              <div class="row">
                <div class="col-sm-1"></div>
                <div class="col-sm-11">
                  <i>Use commas to separate your Zmojis</i>
                </div>
              </div>
            </div>
            <div class="modal-footer">
              <button tabIndex="3" type="submit" class="btn btn-sm btn-danger"
                      @click="self.resetToServer()">
                Reset
              </button>
              <button v-if="self.serverZmojis.length === 0" tabIndex="4" type="submit" class="btn btn-sm btn-info"
                      @click="self.resetToSuggestions()">
                Suggested
              </button>
              <button tabIndex="5" type="submit" class="btn btn-sm btn-success" data-bs-dismiss="modal"
                      @click="self.update()" :disabled="!self.updateValid">
                Update
              </button>
            </div>
          </div>
        </div>
      </div>
      </div>
    `
}

