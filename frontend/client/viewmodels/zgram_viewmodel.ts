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

import {RenderStyle, SearchOrigin, Zephyrgram, ZgramCore, ZgramId} from "../../shared/protocol/zephyrgram";
import {Z2kState} from "../z2kstate";
import {ZgramEditorResult, ZgramEditorViewModel} from "./zgram_editor_viewmodel";
import {escapeQuotes, isQDecodable, qDecode, staticFail} from "../../shared/utility";
import {InitialQuery} from "../InitialQuery";
import {ReactionsViewModel} from "./reactions_viewmodel";
import {DisplayedRenderStyle} from "../util/renderer";
import {DRequest} from "../../shared/protocol/message/drequest";
import {nextTick} from "vue";
import {PlusPlusesViewModel} from "./pluspluses_viewmodel";
import {AddFilterViewModel} from "./add_filter_viewmodel";
import {SessionStatus} from "../session_status";

const moment = require("moment")

export class ZgramViewModel {
    private readonly zgramVersions: ZgramCore[];
    versionSelector: number;
    maxVersion: number;
    numQDecodeApplications: number;
    weaklyHiddenOverride: boolean;
    renderStyleOverride: DisplayedRenderStyle | undefined;
    controlsEnabled: boolean;
    selectedInteraction: SelectedInteraction | undefined;
    addFilterViewModel: AddFilterViewModel | undefined;
    replyViewModel: ZgramEditorViewModel | undefined;
    editZgramViewModel: ZgramEditorViewModel | undefined;
    readonly reactionsViewModel: ReactionsViewModel;
    readonly plusPlusesViewModel: PlusPlusesViewModel;
    refersTo: ZgramId | undefined;
    referLookupRequested: boolean;

    constructor(private state: Z2kState, private readonly sessionStatus: SessionStatus,
        private readonly zgram: Zephyrgram) {
        this.zgramVersions = [zgram.zgramCore];
        this.versionSelector = 0;
        this.maxVersion = 0;
        this.numQDecodeApplications = 0;
        this.weaklyHiddenOverride = false;
        this.renderStyleOverride = undefined;
        this.controlsEnabled = false;
        this.selectedInteraction = undefined;
        this.addFilterViewModel = undefined;
        this.replyViewModel = undefined;
        this.editZgramViewModel = undefined;
        this.reactionsViewModel = new ReactionsViewModel(this, state);
        this.plusPlusesViewModel = new PlusPlusesViewModel(this, state);
        this.refersTo = undefined;
        this.referLookupRequested = false;
    }

    get zgramId() {
        // This value is immutable.
        return this.zgram.zgramId;
    }

    get sender() {
        // This value is immutable.
        return this.zgram.sender;
    }

    get signature() {
        // This value is immutable.
        return this.zgram.signature;
    }

    get isLogged() {
        // This value is immutable.
        return this.zgram.isLogged;
    }

    get instance() {
        return this.zgramVersions[this.versionSelector].instance;
    }

    get thisBodyVersion() {
        return this.zgramVersions[this.versionSelector].body;
    }

    get body() {
        let result = this.thisBodyVersion;
        for (let i = 0; i != this.numQDecodeApplications; ++i) {
            result = qDecode(result);
        }
        return result;
    }

    get abbreviatedBody() {
        return this.thisBodyVersion.substring(0, 60);
    }

    get rawRenderStyle() {
        return this.zgramVersions[this.versionSelector].renderStyle;
    }

    get renderStyle() {
        if (this.renderStyleOverride !== undefined) {
            return this.renderStyleOverride;
        }
        return this.translateRenderStyle(this.rawRenderStyle);
    }

    toggleRendering() {
        const passThroughRenderStyle = this.translateRenderStyle(this.rawRenderStyle);
        const nextRenderStyle = this.nextRenderStyle(this.renderStyle);
        if (nextRenderStyle === passThroughRenderStyle) {
            this.renderStyleOverride = undefined;
        } else {
            this.renderStyleOverride = nextRenderStyle;
        }
    }

    copyLinkToZgram() {
        const url = this.makeUriForQueryOnZgramId();
        navigator.clipboard.writeText(url);
    }

    get weaklyHidden() {
        if (!this.sessionStatus.filtersEnabled) {
            return false;
        }
        if (this.weaklyHiddenOverride) {
            return false;
        }
        return this.state.filtersViewModel.matchesAny(false, this.sender, this.instance);
    }

    get stronglyHidden() {
        if (!this.sessionStatus.filtersEnabled) {
            return false;
        }
        return this.state.filtersViewModel.matchesAny(true, this.sender, this.instance);
    }

    overrideWeaklyHidden() {
        this.weaklyHiddenOverride = true;
    }

    private translateRenderStyle(style: RenderStyle) {
        switch (style) {
            case RenderStyle.Default: return DisplayedRenderStyle.Default;
            case RenderStyle.MarkDeepMathAjax: return DisplayedRenderStyle.MarkDeepMathAjax;
            default: staticFail(style);
        }
    }

    private nextRenderStyle(style: DisplayedRenderStyle) {
        switch (style) {
            case DisplayedRenderStyle.Default: return DisplayedRenderStyle.Monospace;
            case DisplayedRenderStyle.Monospace: return DisplayedRenderStyle.MarkDeepMathAjax;
            case DisplayedRenderStyle.MarkDeepMathAjax: return DisplayedRenderStyle.Default;
            default: staticFail(style);
        }
    }

    get isQDecodeEnabled() {
        return isQDecodable(this.thisBodyVersion);
    }

    performQDecode() {
        if (isQDecodable(this.body)) {
            ++this.numQDecodeApplications;
        } else {
            this.numQDecodeApplications = 0;
        }
    }

    get hasEdits() {
        return this.zgramVersions.length > 1;
    }

    // get maxVersion() {
    //     return this.zgramVersions.length - 1;
    // }

    mouseenter() {
        this.state.currentlyHoveringZgram = this;
    }

    mouseleave() {
        this.state.currentlyHoveringZgram = undefined;
    }

    toggleControls() {
        this.controlsEnabled = !this.controlsEnabled;
    }

    get isAddFilterVisible() {
        return this.selectedInteraction === SelectedInteraction.AddFilter;
    }
    get isReplyVisible() {
        return this.selectedInteraction === SelectedInteraction.Reply;
    }
    get isEditZgramVisible() {
        return this.selectedInteraction === SelectedInteraction.Edit;
    }
    get isReactionsVisible() {
        return this.selectedInteraction === SelectedInteraction.Reactions;
    }

    get hasHiddenReplyDraft() {
        return this.selectedInteraction !== SelectedInteraction.Reply && this.replyViewModel !== undefined;
    }
    get hasHiddenEditDraft() {
        return this.selectedInteraction !== SelectedInteraction.Edit && this.editZgramViewModel !== undefined;
    }

    toggleAddFilterInteraction() {
        if (this.disableAllInteractions(SelectedInteraction.AddFilter)) {
            return;
        }
        if (this.addFilterViewModel === undefined) {
            this.addFilterViewModel = new AddFilterViewModel(this, this.state, this.sender, this.instance);
        }
        this.selectedInteraction = SelectedInteraction.AddFilter;
    }

    get replyVisible() {
        return this.selectedInteraction === SelectedInteraction.Reply;
    }

    toggleReplyInteraction() {
        if (this.disableAllInteractions(SelectedInteraction.Reply)) {
            return;
        }

        if (this.replyViewModel === undefined) {
            const onSubmit = (result: ZgramEditorResult | undefined) => {
                if (result !== undefined) {
                    const zgc = new ZgramCore(result.instance, result.body, result.renderStyle);
                    this.state.postZgram(zgc, this.zgramId);
                    this.replyViewModel = undefined;
                }
                this.disableAllInteractions(SelectedInteraction.Reply);
            };

            // TODO(kosak): get the typing right on textSelection
            const selected = (this.state.textSelection.text as string).trim();
            const indented = selected.replaceAll('\n', '\n> ');
            const body = indented.length === 0 ? "" : `> ${indented}\n`;
            this.replyViewModel = ZgramEditorViewModel.forReply(this.state, this.instance, body, onSubmit);
        }

        this.selectedInteraction = SelectedInteraction.Reply;
    }

    get canEdit() {
        return this.zgram.sender === this.state.sessionStatus.profile.userId;
    }

    toggleEditZgram() {
        if (this.disableAllInteractions(SelectedInteraction.Edit)) {
            return;
        }
        if (this.editZgramViewModel === undefined) {
            const onSubmit = (result: ZgramEditorResult | undefined) => {
                if (result !== undefined) {
                    const mds = ZgramEditorViewModel.calcUpdates(this, result);
                    if (mds.length === 0) {
                        // Shouldn't get here
                        return;
                    }
                    this.state.postMetadata(mds);
                    this.editZgramViewModel = undefined;
                }
                this.disableAllInteractions(SelectedInteraction.Reply);
            };

            this.editZgramViewModel = ZgramEditorViewModel.forEdit(this.state, this, onSubmit);
        }
        this.selectedInteraction = SelectedInteraction.Edit;
    }

    toggleReactionsInteraction() {
        if (this.disableAllInteractions(SelectedInteraction.Reactions)) {
            return;
        }
        this.selectedInteraction = SelectedInteraction.Reactions;
    }

    disableAllInteractions(which: SelectedInteraction) {
        const wasSet = this.selectedInteraction == which;

        this.selectedInteraction = undefined;
        if (this.addFilterViewModel !== undefined) {
            this.addFilterViewModel = undefined;
        }
        if (this.replyViewModel !== undefined && !this.replyViewModel.isDirty) {
            this.replyViewModel = undefined;
        }
        if (this.editZgramViewModel !== undefined && !this.editZgramViewModel.isDirty) {
            this.editZgramViewModel = undefined;
        }
        return wasSet;
    }

    get referredToZgram() : ZgramViewModel | undefined {
        if (this.refersTo === undefined) {
            // No refers-to
            return undefined;
        }

        const zvm = this.state.zgramDict[this.refersTo.raw];
        if (zvm !== undefined) {
            // Have refers-to and the definition of the referred-to zgram. All set
            console.log("returning");
            console.log(zvm);
            return zvm;
        }

        if (!this.referLookupRequested) {
            this.state.requestZgram(this.refersTo);
            this.referLookupRequested = true;
        }
        return undefined;
    }

    get haveReferredToZgram() {
        return this.referredToZgram !== undefined;
    }

    // Used for href links
    makeUriForQueryOnSender() {
        const iq = this.makeQueryOnSender();
        return this.state.makeUriFor(iq);
    }

    makeUriForQueryOnInstance() {
        const iq = this.makeQueryOnInstance();
        return this.state.makeUriFor(iq);
    }

    // Used for href links
    makeUriForQueryOnTimestamp() {
        const iq = this.makeQueryOnTimestamp();
        return this.state.makeUriFor(iq);
    }

    makeUriForQueryOnZgramId() {
        const iq = this.makeQueryOnZgramId();
        return this.state.makeUriFor(iq);
    }

    // are these openXXX methods unused?
    openNewQueryOnInstance() {
        const iq = this.makeQueryOnInstance();
        this.state.openNewQuery(iq);
    }

    openNewQueryOnTimestamp() {
        const iq = this.makeQueryOnTimestamp();
        this.state.openNewQuery(iq);
    }

    doClick() {
        this.state.updateLastReadZgram(this.zgramId);
    }

    doSpeak() {
        const svm = this.state.speechViewModel;
        svm.speak(`${this.sender} on ${this.instance}`);
        svm.speak(this.body);
    }

    private makeQueryOnSender() {
        const searchOrigin = SearchOrigin.ofZgramId(this.zgramId);
        return InitialQuery.ofSender(this.sender, searchOrigin);
    }

    private makeQueryOnInstance() {
        const searchOrigin = SearchOrigin.ofZgramId(this.zgramId);
        return InitialQuery.ofInstance(this.instance, searchOrigin);
    }

    private makeQueryOnTimestamp() {
        const searchOrigin = SearchOrigin.ofTimestamp(this.zgram.timesecs);
        return InitialQuery.ofDefault(searchOrigin);
    }

    private makeQueryOnZgramId() {
        return InitialQuery.ofId(this.zgramId);
    }

    addRevision(newZgc: ZgramCore) {
        this.zgramVersions.push(newZgc);
        this.maxVersion = this.zgramVersions.length - 1;
        // It looks like the range slider does clamping/input validation. So if the slider happens to get
        // the versionSelector increase before the maxVersion increase, it may ignore it. We do this to force
        // the maxVersion increase to happen first.
        nextTick(() => {
            this.versionSelector = this.zgramVersions.length - 1;
        });
    }

    get customerReadableDate() {
        const m = moment.unix(this.zgram.timesecs);
        return m.format("lll");
    }

    get bodyAsHtml() {
        return this.state.renderer.renderBodyAsHtml(this.body, this.renderStyle);
    }

    get isMarkdeepText() {
        return this.renderStyle === DisplayedRenderStyle.MarkDeepMathAjax;
    }

    get isUnread() {
        const lastRead = this.state.lastReadZgramId;
        return lastRead === undefined || this.zgramId.raw > lastRead.raw;
    }

    get isHovering() {
        return this.state.currentlyHoveringZgram === this;
    }

    maybeUpdateMathJax(el: any) {
        if (this.isMarkdeepText) {
            this.state.renderer.updateMathJax(this.body, el);
        }
    }

    // A utility function. Put somewhere else?
    hideZero(value: number) {
        return value === 0 ? "" : value.toString();
    }
}

enum SelectedInteraction {
    AddFilter, Reply, Edit, Reactions
}
