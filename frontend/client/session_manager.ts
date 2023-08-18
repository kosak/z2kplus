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

import {Chunker} from "../shared/chunker";
import {CRequest, crequests} from "../shared/protocol/control/crequest";
import {CResponse, cresponses} from "../shared/protocol/control/cresponse";
import {DRequest} from "../shared/protocol/message/drequest";
import {LinkedList} from "../shared/collections/linked_list";
import {DResponse} from "../shared/protocol/message/dresponse";
import {Profile} from "../shared/protocol/profile";

export enum State {
    Disconnected,
    AwaitingConnect,
    Connected,
    AttachedToSession,
    SessionFailure
}

export class SessionManager {
    private state: State;
    private ws: WebSocket | undefined;
    private unacknowledged: LinkedList<crequests.PackagedRequest>;
    private nextRequestId: number;
    private nextExpectedResponseId: number;
    private assignedSessionId: string | undefined;
    public profile: Profile | undefined;

    private onStateChange: (state: State) => void;
    private onDresponse: (dresp: DResponse) => void;

    constructor() {
        this.state = State.Disconnected;
        this.ws = undefined;
        this.unacknowledged = new LinkedList<crequests.PackagedRequest>();
        this.nextRequestId = 1000;
        this.nextExpectedResponseId = 0;
        this.assignedSessionId = undefined;
        this.profile = undefined;
        this.onStateChange = () => {};
        this.onDresponse = () => {};
    }

    start(onStateChange: (state: State) => void, onDresponse: (dresp: DResponse) => void) {
        this.onStateChange = onStateChange;
        this.onDresponse = onDresponse;
        this.connect();
    }

    private connect() {
        if (this.state !== State.Disconnected) {
            return;
        }
        const url = `wss://${window.location.host}/api/`;
        console.log(`Trying to connect to ${url}`);
        this.ws = new WebSocket(url);
        this.ws.onopen = () => this.handleOpen();
        this.ws.onclose = () => this.closeHelper("close");
        this.ws.onerror = () => this.closeHelper("error");
        const chunker = new Chunker();
        this.ws.onmessage = (ev: MessageEvent) => this.handleMessage(ev, chunker);
        this.setState(State.AwaitingConnect);
    }

    private flushAcknowledgedRequestsUpTo(nextExpectedRequestId: number) {
        while (!this.unacknowledged.empty()) {
            const front = this.unacknowledged.front();
            if (front.requestId >= nextExpectedRequestId) {
                return;
            }
            this.unacknowledged.tryRemoveFront();
        }
    }

    private handleOpen() {
        let cs: CRequest;
        if (this.assignedSessionId === undefined) {
            cs = CRequest.createCreateSession();
        } else {
            cs = CRequest.createAttachToSession(this.assignedSessionId, this.nextExpectedResponseId);
        }
        this.sendCRequest(cs);
        this.setState(State.Connected);
    }

    private closeHelper(reason: string) {
        if (this.state === State.Disconnected) {
            return;
        }
        this.setState(State.Disconnected);
        this.ws = undefined;
        // Try to connect again in 5 seconds
        setTimeout(() => this.connect(), 5 * 1000);
    }

    private handleMessage(ev: MessageEvent, chunker: Chunker) {
        // data might be a partial message, so add it to the Chunker and then take whole pieces out
        const data = ev.data.toString();
        chunker.pushBack(data);
        while (true) {
            const front = chunker.tryUnwrapNext();
            if (front === undefined) {
                break;
            }
            const jsObj = JSON.parse(front);
            const resp = CResponse.tryParseJson(jsObj);
            resp.acceptVisitor(this);
        }
    }

    visitSessionSuccess(payload: cresponses.SessionSuccess) {
        this.assignedSessionId = payload.assignedSessionId;
        this.profile = payload.profile;
        this.flushAcknowledgedRequestsUpTo(payload.nextExpectedRequestId);
        this.setState(State.AttachedToSession);
        this.catchUp();
    }

    visitSessionFailure(payload: cresponses.SessionFailure) {
        this.setState(State.SessionFailure);
    }

    visitPackagedResponse(presp: cresponses.PackagedResponse) {
        if (presp.responseId !== this.nextExpectedResponseId) {
            console.log(`Expected id ${this.nextExpectedResponseId}, got ${presp.responseId}. Ignoring.`);
            return;
        }
        ++this.nextExpectedResponseId;
        this.flushAcknowledgedRequestsUpTo(presp.nextExpectedRequestId);
        this.onDresponse(presp.response);
    }

    private catchUp() {
        if (this.state !== State.AttachedToSession) {
            return;
        }
        for (const item of this.unacknowledged.entries()) {
            this.sendPackagedRequest(item);
        }
    }

    sendDRequest(req: DRequest) {
        const pr = new crequests.PackagedRequest(this.nextRequestId, this.nextExpectedResponseId, req);
        ++this.nextRequestId;

        this.unacknowledged.append(pr);
        if (this.state !== State.AttachedToSession) {
            return;
        }
        this.sendPackagedRequest(pr);
    }

    private sendCRequest(req: CRequest) {
        const s = JSON.stringify(req.toJson());
        const wrapped = Chunker.wrap(s);
        this.ws!.send(wrapped);
    }

    private sendPackagedRequest(preq: crequests.PackagedRequest) {
        const cr = CRequest.createPackagedRequestFromPayload(preq);
        this.sendCRequest(cr);
    }

    private setState(newState: State) {
        if (this.state === newState) {
            return;
        }
        this.state = newState;
        this.onStateChange(newState);
    }

    get numPendingRequests() {
        return this.unacknowledged.size;
    }
}
