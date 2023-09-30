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
import {Unit} from "../../shared/protocol/misc";
import {escapeQuotes} from "../../shared/utility";
import {MetadataRecord, SearchOrigin} from "../../shared/protocol/zephyrgram";
import {InitialQuery} from "../InitialQuery";
import {ZgramViewModel} from "./zgram_viewmodel";
import {ZmojiViewModel} from "./zmojis_editor_viewmodel";

class InnerMapAndPosition {
    constructor(public readonly map: {[creator: string]: Unit}, public readonly position: number) {}
}

export class ReactionsViewModel {
    reactionText: string;
    private readonly allReactions: {[reaction: string]: InnerMapAndPosition};
    private nextFreePosition: number;
    private readonly myUserId: string;

    constructor(private readonly owner: ZgramViewModel, private readonly state: Z2kState) {
        this.reactionText = "";
        this.allReactions = {};
        this.nextFreePosition = 0;
        this.myUserId = state.sessionStatus.profile.userId;
    }

    updateReactions(reaction: string, creator: string, value: boolean) {
        if (!value) {
            this.removeReaction(reaction, creator);
        } else {
            this.addReaction(reaction, creator);
        }
    }

    private addReaction(reaction: string, creator: string) {
        let inner = this.allReactions[reaction];
        if (inner === undefined) {
            this.allReactions[reaction] = inner = new InnerMapAndPosition({}, this.nextFreePosition++);
        }
        inner.map[creator] = Unit.create();
    }

    private removeReaction(reaction: string, creator: string) {
        const inner = this.allReactions[reaction];
        if (inner === undefined) {
            return;
        }
        delete inner.map[creator];
        // Normally we would test to see if inner.length === 0 and if so, delete the inner dictionary.
        // HOWEVER if the remove is due to us, we like the effect of leaving an empty slot around in case
        // the remove was done by the customer fat-fingering the wrong button.
        if (creator !== this.myUserId && Object.keys(inner.map).length === 0) {
            delete this.allReactions[reaction];
        }
    }

    getReactionsForZgramHeader() {
        const onClick = (rvm: ReactionViewModel) => {
            const escaped = escapeQuotes(rvm.text);
            const queryText = `hasreaction("${escaped}")`;
            const searchOrigin = SearchOrigin.ofZgramId(this.owner.zgramId);
            const query = new InitialQuery(queryText, searchOrigin);
            this.state.openNewQuery(query);
        }
        return this.getReactions(onClick);
    }

    getReactionsForZgramBody() {
        const onClick = (rvm: ReactionViewModel) => {
            // If this reaction includes me, then remove me. Otherwise add me.
            const md = MetadataRecord.createReaction(this.owner.zgramId, rvm.text, this.myUserId, !rvm.includesMe);
            this.state.postMetadata([md]);
        }
        return this.getReactions(onClick);
    }

    getReactionsForReactionInteraction() {
        const onClick = (rvm: ReactionViewModel) => {
            // If this reaction includes me, then remove me. Otherwise add me.
            const md = MetadataRecord.createReaction(this.owner.zgramId, rvm.text, this.myUserId, !rvm.includesMe);
            this.state.postMetadata([md]);
        }
        return this.getReactions(onClick);
    }

    getZmojisForReactions() {
        const onClick = (text: string) => {
            const md = MetadataRecord.createReaction(this.owner.zgramId, text, this.myUserId, true);
            this.state.postMetadata([md]);
        }
        const zmojis = this.state.getZmojis(onClick);

        // Hackjob: grey out the zmojis that we've already applied to this zgram
        const result: ZmojiViewModelWithEnabled[] = [];
        for (const zvm of zmojis) {
            const inner = this.allReactions[zvm.text];
            const enabled = inner === undefined || inner.map[this.myUserId] === undefined;
            result.push(new ZmojiViewModelWithEnabled(zvm, enabled));
        }
        return result;
    }

    submitNewReaction() {
        console.log("hello, submitting new reaction");
        const trimmed = this.reactionText.trim();
        const md = MetadataRecord.createReaction(this.owner.zgramId, trimmed, this.myUserId, true);
        this.reactionText = "";
        this.state.postMetadata([md]);
    }

    get okToSubmitNewReaction() {
        return this.reactionText.trim().length !== 0;
    }

    get numReactions() {
        return Object.keys(this.allReactions).length;
    }

    private getReactions(onClick: (rvm: ReactionViewModel) => void,
                         onUpvote: (rvm: ReactionViewModel) => void) {
        const result: ReactionViewModel[] = [];
        for (const [reaction, mapAndPosition] of Object.entries(this.allReactions)) {
            const map = mapAndPosition.map;
            const position = mapAndPosition.position;
            const includesMe = map[this.myUserId] !== undefined;
            const mapSize = Object.keys(map).length;
            result.push(new ReactionViewModel(position, reaction, mapSize, includesMe,
                onClick, onUpvote));
        }
        result.sort((a, b) => a.stablePosition - b.stablePosition);
        return result;
    }
}

class ReactionViewModel {
    constructor(private readonly owner: ZgramViewModel, private readonly state: Z2kState,
        readonly stablePosition: number, readonly text: string, readonly count: number,
        readonly includesMe: boolean) {
    }

    doClick() {
        const escaped = escapeQuotes(this.text);
        const queryText = `hasreaction("${escaped}")`;
        const searchOrigin = SearchOrigin.ofZgramId(this.owner.zgramId);
        const query = new InitialQuery(queryText, searchOrigin);
        this.state.openNewQuery(query);
    }

    doUpvote() {
        this.onUpvote(this);
    }
}

export class ZmojiViewModelWithEnabled {
    constructor(public readonly zvm: ZmojiViewModel, public readonly enabled: boolean) {}

    get text() {
        return this.zvm.text;
    }

    click() {
        this.zvm.click();
    }
}
