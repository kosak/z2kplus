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
import {zgramHeaderComponent} from "./zgram_header_component";
import {zgramBodyComponent} from "./zgram_body_component";
import {zgramFooterComponent} from "./zgram_footer_component";

export const zgramComponent = {
    components: {
        zgramHeaderComponent,
        zgramBodyComponent,
        zgramFooterComponent
    },
    props: {
        zg: ZgramViewModel
    },
    template: `
      <div class="row">
      annoying
      <div class="col-8"
           v-if="!zg.stronglyHidden" v-on:mouseenter="zg.mouseenter()" v-on:mouseleave="zg.mouseleave()">
        <zgram-header-component :zg="zg">
        </zgram-header-component>
        <zgram-body-component :zg="zg">
        </zgram-body-component>
        <zgram-footer-component :zg="zg">
        </zgram-footer-component>
      </div>
      </div>
    `
}
