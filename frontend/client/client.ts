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

import {createApp} from 'vue';
import {reactive} from 'vue';
import {Z2kState} from "./z2kstate";
import {filtersDialogComponent} from "./components/filters_dialog_component";
import {queryDialogComponent} from "./components/query_dialog_component";
import {speechDialogComponent} from "./components/speech_dialog_component";
import {streamStatusComponent} from "./components/stream_status_component";
import {zgramComponent} from "./components/zgram_component";
import {zgramEditorComponent} from "./components/zgram_editor_component";
import {zmojisEditorDialogComponent} from "./components/zmojis_editor_dialog_component";

const state = reactive(new Z2kState());
state.start();

const z2kApp = {
  components: {
    filtersDialogComponent,
    queryDialogComponent,
    speechDialogComponent,
    streamStatusComponent,
    zgramComponent,
    zgramEditorComponent,
    zmojisEditorDialogComponent
  },

  data() {
    return {
      state: state,
      status: state.sessionStatus
    }
  },

  watch: {
    "state.documentTitle"(newTitle: string) {
      document.title = newTitle;
    }
  }
}

const foo = createApp(z2kApp);
foo.mount("#z2kApp");
