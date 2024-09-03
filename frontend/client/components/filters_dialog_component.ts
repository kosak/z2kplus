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

import {FiltersViewModel} from "../viewmodels/filters_viewmodel";

export const filtersDialogComponent = {
    props: {
        self: FiltersViewModel
    },
    template: `
      <div ref="modal" id="filtersDialog" class="modal modal-lg fade" tabindex="-1">
      <div class="modal-dialog">
        <div class="modal-content">
          <div class="modal-header">
            <h5 class="modal-title">Manage Filters</h5>
            <button tabIndex="-1" type="button" class="btn-close" data-bs-dismiss="modal">
            </button>
          </div>
          <div class="modal-body">
            <div class="container-fluid">
              <div class="row">
                <div class="col-sm-2"><b>Remove</b></div>
                <div class="col-sm-5"><b>Filter</b></div>
                <div class="col-sm-5"><b>Expiration</b></div>
              </div>

              <div v-for="item in self.allFilterViewModels" class="row">
                <div class="col-sm-2">
                  <button type="button" class="btn-close" @click="item.removeSelf()"></button>
                </div>
                <div class="col-sm-5">{{ item.humanReadableDescription }}</div>
              </div>
            </div>
          </div>
        </div>
      </div>
      </div>
    `
}
