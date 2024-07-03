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
import {zgramBodyComponent} from "./zgram_body_component";
import {zgramFooterComponent} from "./zgram_footer_component";
import {zgramHeaderComponent} from "./zgram_header_component";
import {zgramRefersToComponent} from "./zgram_refers_to_component";

export const zgramComponent = {
    components: {
        zgramHeaderComponent,
        zgramRefersToComponent,
        zgramBodyComponent,
        zgramFooterComponent
    },
    props: {
        zg: ZgramViewModel
    },
    template: `
      <div v-if="!zg.stronglyHidden">
      <div class="row">
        <div class="col-12">
          <zgram-header-component :zg="zg">
          </zgram-header-component>
        </div>
      </div>
      <div class="row" v-if="zg.controlsEnabled && zg.haveReferredToZgram">
        <div class="col-12">
          <zgram-refers-to-component :zg="zg.referredToZgram">
          </zgram-refers-to-component>
        </div>
      </div>
      <div class="row" style="max-width:1024px">
        <div class="col-12">
          <zgram-body-component :zg="zg" v-on:mouseenter="zg.mouseenter()">
          </zgram-body-component>
        </div>
      </div>
      <div class="row">
        <div class="col-12">
          <zgram-footer-component :zg="zg">
          </zgram-footer-component>
        </div>
      </div>
      </div>
    `
}
