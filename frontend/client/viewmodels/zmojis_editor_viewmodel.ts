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

import {Z2kState} from "../z2kstate";
import {MetadataRecord} from "../../shared/protocol/zephyrgram";

const suggestedZmojis = "❤,️🍉,🍔,👱🏾,ᶜᶤᶰᶰᵃᵇᵒᶰ";

export class ZmojisEditorViewModel {
    // Currently editing
    editingZmojis: string;
    // As known by the server
    serverZmojis: string;

    constructor(private readonly owner: Z2kState) {
        this.setFromServer("");
    }

    setFromServer(zmojis: string) {
        this.editingZmojis = zmojis;
        this.serverZmojis = zmojis;
    }

    resetToServer() {
        this.editingZmojis = this.serverZmojis;
    }

    resetToSuggestions() {
        this.editingZmojis = suggestedZmojis;
    }

    get updateValid() {
        return this.editingZmojis !== this.serverZmojis;
    }

    update() {
        if (!this.updateValid) {
            return;
        }
        const userId = this.owner.sessionStatus.profile.userId;
        const md = MetadataRecord.createZmojis(userId, this.editingZmojis);
        this.owner.postMetadata([md]);
    }

    getZmojis(onClick: (text: string) => void) {
        const zmojiText = this.serverZmojis.trim();
        if (zmojiText.length === 0) {
            return [];
        }
        return zmojiText.split(',').map(text => new ZmojiViewModel(text, onClick));
    }
}

export class ZmojiViewModel {
    constructor(public readonly text: string, private readonly onClick: (text: string) => void) {}

    click() {
        this.onClick(this.text);
    }
}
