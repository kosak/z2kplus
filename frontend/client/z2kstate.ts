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

import * as URI from "urijs";

import {DRequest} from "../shared/protocol/message/drequest";
import {magicConstants} from "../shared/magic_constants";
import {SessionManager, State as SessionManagerState} from "./session_manager";
import {DResponse, dresponses} from "../shared/protocol/message/dresponse";
import {Estimates} from "../shared/protocol/misc";
import {
    MetadataRecord,
    RenderStyle,
    searchOriginInfo,
    userMetadata,
    zgMetadata,
    ZgramCore,
    ZgramId
} from "../shared/protocol/zephyrgram";
import {SessionStatus} from "./session_status";
import {ZgramViewModel} from "./viewmodels/zgram_viewmodel";
import {ZgramEditorResult, ZgramEditorViewModel} from "./viewmodels/zgram_editor_viewmodel";
import {QueryViewModel} from "./viewmodels/query_viewmodel";
import {SpeechViewmodel} from "./viewmodels/speech_viewmodel";
import {ZmojisEditorViewModel} from "./viewmodels/zmojis_editor_viewmodel";
import {InitialQuery} from "./InitialQuery";
import {StreamStatusViewModel} from "./viewmodels/stream_status_viewmodel";
import {useActiveElement, useMagicKeys, useTextSelection, whenever} from '@vueuse/core'
import {UploadableMediaUtil} from "../shared/uploadable_media_util";
import {Renderer} from "./util/renderer";
import {FiltersViewModel} from "./viewmodels/filters_viewmodel";
import {Optional, Pair} from "../shared/utility";

export class Z2kState {
    readonly host: string;
    readonly sessionManager : SessionManager;
    readonly sessionStatus: SessionStatus;
    readonly zgrams: ZgramViewModel[];
    lastReadZgramId: ZgramId | undefined;
    composeViewModel: ZgramEditorViewModel | undefined;
    composeVisible: boolean;
    readonly queryViewModel: QueryViewModel;
    readonly zmojisEditorViewModel: ZmojisEditorViewModel;
    readonly speechViewModel: SpeechViewmodel;
    readonly filtersViewModel: FiltersViewModel;
    readonly frontStreamStatus: StreamStatusViewModel;
    readonly backStreamStatus: StreamStatusViewModel;
    currentlyHoveringZgram: ZgramViewModel | undefined;
    readonly zgramDict: {[rawId: number]: ZgramViewModel};
    private nextFakeId: number;
    readonly uploadableMediaUtil: UploadableMediaUtil;
    readonly renderer: Renderer;
    // TODO: fix type
    readonly textSelection: any;

    constructor() {
        this.host = window.location.host;
        this.sessionManager = new SessionManager();
        this.sessionStatus = new SessionStatus();
        this.zgrams = [];
        this.lastReadZgramId = undefined;
        this.composeViewModel = undefined;
        this.composeVisible = false;
        this.queryViewModel = new QueryViewModel(this);
        this.zmojisEditorViewModel = new ZmojisEditorViewModel(this);
        this.speechViewModel = new SpeechViewmodel();
        this.filtersViewModel = new FiltersViewModel();
        this.frontStreamStatus = new StreamStatusViewModel("ss-front", this, false);
        this.backStreamStatus = new StreamStatusViewModel("ss-back", this, true);
        this.currentlyHoveringZgram = undefined;
        this.zgramDict = {};
        this.nextFakeId = 0;
        this.uploadableMediaUtil = new UploadableMediaUtil();
        this.renderer = new Renderer(this);
        this.textSelection = useTextSelection();
    }

    // This stuff runs after this object has been made reactive. Take care to have stuff like callbacks here rather than
    // in the constructor (because, if you do it in the constructor, you will have the wrong "this")
    start() {
        // Support hotkeys C and R for compose and reply, respectively. Super hack job.
        const activeElement = useActiveElement();
        const activeElementIsInput = () => {
            const tagName = activeElement.value?.tagName;
            return tagName === "INPUT" || tagName === "TEXTAREA";
        };
        const isKeyWithoutModifier = (e: KeyboardEvent, expected: string) => {
            return e.key === expected && e.type === 'keydown' && !e.shiftKey && !e.ctrlKey && !e.altKey && !e.metaKey;
        };
        const handleEventFired = (e: KeyboardEvent, expected: string) => {
            if (!activeElementIsInput() && isKeyWithoutModifier(e, expected)) {
                e.preventDefault();
            }
        };
        const {shift, ctrl, alt, meta} = useMagicKeys();

        const noModifiers = () => {
            return !shift.value && !ctrl.value && !alt.value && !meta.value;
        };

        const {c} = useMagicKeys({
            passive: false,
            onEventFired(e) {
                handleEventFired(e, 'c');
            },
        });
        const {r} = useMagicKeys({
            passive: false,
            onEventFired(e) {
                handleEventFired(e, 'r');
            },
        });
        whenever(c, () => {
            if (noModifiers() && !activeElementIsInput()) {
                this.toggleComposeViewModel();
            }
        });
        whenever(r, () => {
            if (noModifiers() && !activeElementIsInput() && this.currentlyHoveringZgram !== undefined) {
                this.currentlyHoveringZgram.toggleReplyInteraction();
            }
        });

        window.onbeforeunload = () => {
            if (activeElementIsInput() || this.numPendingRequests !== 0) {
                return "Are you sure you want to leave?";
            }
        };

        this.sessionManager.start( s => this.handleStateChange(s), d => this.handleDresponse(d));
        this.sessionStatus.queryOutstanding = true;
        const iq = InitialQuery.createFromLocationOrDefault(document.location);
        this.queryViewModel.resetToIq(iq);
        if (iq.searchOrigin.tag === searchOriginInfo.Tag.End) {
            this.frontStreamStatus.setAppetite(magicConstants.initialQuerySize);
            this.backStreamStatus.setAppetite(undefined);
        } else {
            const half = magicConstants.initialQuerySize / 2;
            this.frontStreamStatus.setAppetite(half);
            this.backStreamStatus.setAppetite(half);
        }
        const sub = DRequest.createSubscribe(iq.query, iq.searchOrigin, magicConstants.pageSize,
            magicConstants.queryMargin);
        this.sessionManager.sendDRequest(sub);
    }

    loadNewPageWithDefaultQuery() {
        window.open(window.location.origin);
    }

    toggleComposeViewModel() {
        if (this.composeVisible) {
            this.composeVisible = false;
            if (this.composeViewModel !== undefined && !this.composeViewModel.isDirty) {
                this.composeViewModel = undefined;
            }
            return;
        }

        if (this.composeViewModel === undefined) {
            const onSubmit = (result: ZgramEditorResult | undefined) => {
                if (result !== undefined) {
                    const zgc = new ZgramCore(result.instance, result.body, result.renderStyle);
                    this.postZgram(zgc, undefined);
                    this.composeViewModel = undefined;
                }
                this.composeVisible = false;
            };
            this.composeViewModel = ZgramEditorViewModel.forCompose(this, onSubmit);
        }
        this.composeVisible = true;
    }

    fakePost() {
        const zg = new ZgramCore("graffiti.test", "Test Message " + this.nextFakeId, RenderStyle.Default);
        ++this.nextFakeId;
        this.postZgram(zg, undefined);
    }

    postZgram(zgram: ZgramCore, refersTo: ZgramId | undefined) {
        const zgPairs = [Pair.create(zgram, Optional.create(refersTo))];
        const post = DRequest.createPostZgrams(zgPairs);
        this.sessionManager.sendDRequest(post);
    }

    requestZgram(zgramId: ZgramId) {
        const req = DRequest.createGetSpecificZgrams([zgramId]);
        this.sessionManager.sendDRequest(req);
    }

    postMetadata(mdrs: MetadataRecord[]) {
        const post = DRequest.createPostMetadata(mdrs);
        this.sessionManager.sendDRequest(post);
    }

    performSyntaxCheck(query: string) {
        const cs = DRequest.createCheckSyntax(query);
        this.sessionManager.sendDRequest(cs);
    }

    openNewQuery(query: InitialQuery) {
        const uri = this.makeUriFor(query);
        // _blank is the special target which means new (tab or window, but I think always tab).
        window.open(uri, "_blank");
    }

    makeUriFor(query: InitialQuery) {
        const iqJson = JSON.stringify(query.toJson());
        // Surgically modify my current URL and put query=XXX in the search section.
        const uri = new URI(document.location);
        uri.search({query: iqJson});
        return uri.toString();
    }

    private handleStateChange(state: SessionManagerState) {
        this.sessionStatus.online = false;  // Pessimistically assume false.
        switch (state) {
            case SessionManagerState.AttachedToSession:
                this.sessionStatus.profile = this.sessionManager.profile!;
                this.sessionStatus.online = true;
                break;

            case SessionManagerState.SessionFailure:
                window.location.reload();
                break;
        }
    }

    private handleDresponse(resp: DResponse) {
        resp.acceptVisitor(this);
    }

    visitAckSyntaxCheck(resp: dresponses.AckSyntaxCheck) {
        // Just forward these all to the query dialog
        this.queryViewModel.onAckSyntaxCheck(resp)
    }

    visitAckSubscribe(resp: dresponses.AckSubscribe) {
        this.sessionStatus.queryOutstanding = false;
        this.processEstimatesUpdate(resp.estimates);
    }

    visitAckMoreZgrams(resp: dresponses.AckMoreZgrams) {
        this.sessionStatus.getMoreZgramsRequestOutstanding = false;
        if (resp.zgrams.length === 0) {
            return;
        }
        const wrappedZgrams = resp.zgrams.map(zg => new ZgramViewModel(this, zg));
        for (const zvm of wrappedZgrams) {
            this.zgramDict[zvm.zgramId.raw] = zvm;
        }
        let wantUpdate: boolean;
        let wantSpeech: boolean;
        if (!resp.forBackSide) {
            wrappedZgrams.reverse();
            this.zgrams.unshift(...wrappedZgrams);
            this.frontStreamStatus.reduceAppetite(wrappedZgrams.length);
            // front side zgrams always update the "last read" zgram.
            // Meaning, new front side zgrams always get rendered as "having been read".
            wantUpdate = true;
            wantSpeech = this.frontStreamStatus.wantSpeech;
        } else {
            this.zgrams.push(...wrappedZgrams);
            this.backStreamStatus.reduceAppetite(wrappedZgrams.length);
            // back side zgrams only update the "last read" zgram when we are not streaming.
            // Meaning, new back side zgrams always get rendered as "having been read", unless
            // we're streaming.
            wantUpdate = !this.backStreamStatus.isStreaming;
            wantSpeech = this.backStreamStatus.wantSpeech;
        }
        if (wantUpdate) {
            this.updateLastReadZgram(wrappedZgrams[wrappedZgrams.length - 1].zgramId);
        }
        this.processEstimatesUpdate(resp.estimates);
        if (wantSpeech) {
            for (const zg of wrappedZgrams) {
                zg.doSpeak();
            }
        }
    }

    updateLastReadZgram(zgramId: ZgramId) {
        if (this.lastReadZgramId === undefined || this.lastReadZgramId.raw < zgramId.raw) {
            this.lastReadZgramId = zgramId;
        }
    }

    // A reactive property (that we watch with a watcher) to inefficiently count the number of unread zgrams
    // and then generate the document title.
    get documentTitle() {
        let numUnread: number;
        if (this.lastReadZgramId === undefined) {
            numUnread = this.zgrams.length;
        } else {
            numUnread = 0;
            for (let ri = this.zgrams.length - 1; ri >= 0; --ri) {
                if (this.zgrams[ri].zgramId.raw <= this.lastReadZgramId.raw) {
                    break;
                }
                ++numUnread;
            }
        }
        const brand = "Z2K Plus+";
        if (numUnread === 0) {
            return brand;
        }
        return `[${numUnread}] ${brand}`;
    }

    visitEstimatesUpdate(resp: dresponses.EstimatesUpdate) {
        this.processEstimatesUpdate(resp.estimates);
    }

    visitMetadataUpdate(resp: dresponses.MetadataUpdate) {
        for (const md of resp.metadata) {
            md.acceptVisitor(this);
        }
    }

    visitAckSpecificZgrams(resp: dresponses.AckSpecificZgrams) {
        for (const zg of resp.zgrams) {
            const raw = zg.zgramId.raw;
            if (this.zgramDict[raw] !== undefined) {
                continue;
            }
            this.zgramDict[raw] = new ZgramViewModel(this, zg);
        }
    }

    visitPlusPlusUpdate(resp: dresponses.PlusPlusUpdate) {
        for (const [zgramId, key, value] of resp.entries) {
            const zgv = this.zgramDict[zgramId.raw];
            if (zgv === undefined) {
                continue;
            }
            zgv.plusPlusesViewModel.addPlusPlusCount(key, value);
        }
    }

    visitAckPing(resp: dresponses.AckPing) {
        console.log("AckPing - todo");
    }

    visitGeneralError(resp: dresponses.GeneralError) {
        console.log(`Error from server: ${resp.message}`);
    }

    visitReaction(rx: zgMetadata.Reaction) {
        const zvm = this.zgramDict[rx.zgramId.raw];
        if (zvm === undefined) {
            // Shouldn't happen.
            return;
        }
        zvm.reactionsViewModel.updateReactions(rx.reaction, rx.creator, rx.value);
    }

    visitZgramRevision(zgRev: zgMetadata.ZgramRevision) {
        const zvm = this.zgramDict[zgRev.zgramId.raw];
        if (zvm === undefined) {
            // Shouldn't happen.
            return;
        }
        zvm.addRevision(zgRev.zgc);
    }

    visitZgramRefersTo(zgRefersTo: zgMetadata.ZgramRefersTo) {
        const zvm = this.zgramDict[zgRefersTo.zgramId.raw];
        if (zvm === undefined) {
            // Shouldn't happen.
            return;
        }
        zvm.refersTo = zgRefersTo.refersTo;
    }

    visitZmojis(zmojis: userMetadata.Zmojis) {
        this.zmojisEditorViewModel.setFromServer(zmojis.zmojis);
    }

    getZmojis(onClick: (text: string) => void) {
        return this.zmojisEditorViewModel.getZmojis(onClick);
    }

    private processEstimatesUpdate(estimates: Estimates) {
        const fs = this.frontStreamStatus;
        const bs = this.backStreamStatus;
        fs.estimate = estimates.front;
        bs.estimate = estimates.back;
        this.sendMoreIfThereIsSufficientAppetite();
    }

    sendMoreIfThereIsSufficientAppetite() {
        if (this.sessionStatus.getMoreZgramsRequestOutstanding) {
            return;
        }
        let backSide = true;
        let count = this.backStreamStatus.calcGetMoreAmount();
        if (count === 0) {
            backSide = false;
            count = this.frontStreamStatus.calcGetMoreAmount();
            if (count === 0) {
                return;
            }
        }
        this.sessionStatus.getMoreZgramsRequestOutstanding = true;
        const req = DRequest.createGetMoreZgrams(backSide, count);
        this.sessionManager.sendDRequest(req);
    }

    get numPendingRequests() {
        return this.sessionManager.numPendingRequests;
    }
}
