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

import {QueryViewModel} from "../viewmodels/query_viewmodel";

export const queryDialogComponent = {
    props: {
        self: QueryViewModel
    },
    mounted() {
        this.$refs.modal.addEventListener('shown.bs.modal', () => {
            this.$refs.query.focus();
            this.self.onFocused();
        });
    },
    watch: {
        'self.query'(newValue: string, oldValue: string) {
            this.self.onQueryChanged(newValue);
        }
    },
    template: `
      <div ref="modal" id="queryDialog" class="modal modal-lg fade" tabindex="-1">
      <div class="modal-dialog">
        <div class="modal-content">
          <div class="modal-header">
            <h5 class="modal-title">Query</h5>
            <button tabIndex="-1" type="button" class="btn-close" data-bs-dismiss="modal">
            </button>
          </div>
          <div class="modal-body">
            <div class="container-fluid">
              <i><label>Query</label></i>
              <span v-if="self.invalid" class="badge bg-danger">Parse Error</span>
              <!-- we are using the textInput databinding in order to get updates on every char -->
              <input ref="query" tabIndex="1" class="form-control" placeholder="Expression" v-model="self.query">
              <div class="row">
                <div class="col-sm-2">Start at</div>
                <div class="col-sm-10">
                  <div class="form-check form-check-inline">
                    <label class="form-check-label">
                      <input tabIndex="-1" class="form-check-input" type="radio" v-model="self.startAt" value="Begin">
                      beginning
                    </label>
                  </div>
                  <div class="form-check form-check-inline">
                    <label class="form-check-label">
                      <input tabIndex="-1" class="form-check-input" type="radio" v-model="self.startAt" value="End">
                      end
                    </label>
                  </div>
                  <div class="form-check form-check-inline">
                    <label class="form-check-label">
                      <input tabIndex="-1" class="form-check-input" type="radio" v-model="self.startAt" value="ZgramId">
                      zgram id
                    </label>
                  </div>
                  <div class="form-check form-check-inline">
                    <label class="form-check-label">
                      <input tabIndex="-1" class="form-check-input" type="radio" v-model="self.startAt"
                             value="Timestamp">
                      timestamp
                    </label>
                  </div>
                </div>
              </div>
              <div  v-if="self.startAt === 'ZgramId'" class="row">
                <div class="col-sm-2">Zgram ID</div>
                <div class="col-sm-10">
                <input tabIndex="-1" type="text" v-model="self.specifiedZgramId" placeholder="ZgramId">
                </div>
              </div>
              <div  v-if="self.startAt === 'Timestamp'" class="row">
                <div class="col-sm-2">Timestamp</div>
                <div class="col-sm-10">
                  <input tabIndex="-1" type="text" v-model="self.specifiedTimestamp" placeholder="Timestamp">
                </div>
              </div>
            </div>

          </div>
          <div class="modal-footer">
            <button tabIndex="-1" type="reset" class="btn btn-sm btn-outline-dark" @click="self.reset()">
              Clear
            </button>
            <button tabIndex="3" type="submit" class="btn btn-sm btn-success" data-bs-dismiss="modal"
                    @click="self.performQuery()" :disabled="!self.valid">
              Query
            </button>
          </div>
        </div>
      </div>
      </div>
    `
};
