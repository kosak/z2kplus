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

import {zgramEditorPreviewComponent} from "./zgram_editor_preview_component";
import {ZgramEditorViewModel} from "../viewmodels/zgram_editor_viewmodel";

export const zgramEditorComponent = {
    components: {
      zgramEditorPreviewComponent
    },
    mounted() {
        // When you open a compose interaction, the instance should get focus first (because it is empty).
        // When you open a reply interaction, the body should get focus (whether or not it is empty, because
        // the instance is already (likely) set, and you probably want to default to body, even though you
        // can edit instance if you want.
        if (this.self.instance.length === 0) {
            this.$refs.instance.focus();
        } else {
            this.$refs.body.focus();
        }
    },
    props: {
        self: ZgramEditorViewModel
    },
    template: `
<div class="container-fluid" @keydown.esc="self.close()">
    <!-- instance field -->
    <div class="form-group row">
      <label for="composeInstance" class="col-sm-1 col-form-label" 
             :class="{zgramUnloggedBody: !self.logged}">
        {{ self.humanReadableMode }} Instance
      </label>
      <div class="col-sm-11">
        <input ref="instance"
               tabindex="1"
               v-model="self.instance"
               class="form-control"
               :class="{zgramUnloggedBody: !self.logged}"
               placeholder="Instance">
      </div>
    </div>
    <!-- message field -->
    <div class="form-group row">
      <label class="col-sm-1 col-form-label"
             :class="{zgramUnloggedBody: !self.logged}">
        {{ self.humanReadableMode }} Message
      </label>
      <div class="col-sm-11">
            <textarea ref="body"
                      tabindex="2"
                      v-model="self.body"
                      class="form-control form-control-sm" rows="10"
                      placeholder="Enter your message here"
                      @keydown.ctrl.enter="self.submit()"
                      @keyup="self.onKeyOrMouseUpForSelection($event)"
                      @mouseup="self.onKeyOrMouseUpForSelection($event)"
                      :class="{'kosak-font-mono': self.wantMarkdeepMathJax}">
            </textarea>
      </div>
    </div>
    
    <!-- Zmojis -->
    <div class="form-group row">
      <div class="col-sm-1">Zmojis:</div>
      <div class="col-sm-11">
        <div>
          <button v-for="zmoji in self.getZmojis()"
                  class="btn btn-sm btn-outline-dark"
                  @click="zmoji.click()">
            {{ zmoji.text }}
          </button>
        </div>
      </div>
    </div>

    <div class="form-group row">
      <div class="col-sm-1">Options:</div>
      <div class="col-sm-11">
        <div class="form-check">
          <label class="form-check-label">
            <input type="checkbox" class="form-check-input" v-model="self.wantMarkdeepMathJax">
            Render with Markdeep / MathJax
          </label>
        </div>
      </div>
    </div>

    <!-- Preview -->
    <zgram-editor-preview-component v-if="self.wantMarkdeepMathJax" :self="self">
    </zgram-editor-preview-component>
    
    <!-- row of buttons -->
    <div>
    <!-- Reset -->
    <button tabindex="-1" type="reset" class="btn btn-sm btn-outline-dark" @click="self.doReset()">
      Reset
    </button>

    <!-- Q-Encode -->
    <button tabindex="-1" class="btn btn-sm btn-outline-dark" @click="self.qEncode()">
      {{ self.qEncodeLabel }}
    </button>

    <!-- Insert Markdeep/MathJax examples -->
    <button tabindex="-1" class="btn btn-sm btn-outline-dark"
            @click="self.insertMarkdeepExamples()">
      MD Examples
    </button>

    <!-- Paste image -->
    <button tabindex="-1" class="btn btn-sm btn-outline-dark"
            @click="self.pasteImage()">
      Paste Image
    </button>

    <!-- Submit -->
    <button tabindex="4" type="submit" class="btn btn-sm btn-success"
            @click="self.submit()" :disabled="!self.okToSubmit">
      Send
    </button>
    </div>

    <div v-if="self.uploadStatus !== undefined" class="text-danger">
      {{ self.uploadStatus }}
    </div>
</div>
`
}
