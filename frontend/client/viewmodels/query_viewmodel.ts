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
import {SearchOrigin, ZgramId} from "../../shared/protocol/zephyrgram";
import {staticFail} from "../../shared/utility";
import {RateLimiter} from "../../shared/rate_limiter";
import {magicConstants} from "../../shared/magic_constants";
import {dresponses} from "../../shared/protocol/message/dresponse";
import AckSyntaxCheck = dresponses.AckSyntaxCheck;
import {InitialQuery} from "../InitialQuery";
const moment = require("moment")

// Strings because these are accessed by the HTML
enum StartAt { Begin = "Begin", End = "End", ZgramId = "ZgramId", Timestamp = "Timestamp" }
enum Validity { Invalid, Unknown, Valid }

export class QueryViewModel {
    query: string;
    startAt: StartAt;
    specifiedZgramId: string;
    specifiedTimestamp: string;
    lastValidation: AckSyntaxCheck;
    private readonly rateLimiter: RateLimiter;

    constructor(private readonly owner: Z2kState) {
        this.rateLimiter = new RateLimiter(magicConstants.parseCheckIntervalMsec);
        this.reset();
    }

    reset() {
        this.query = "";
        this.startAt = StartAt.End;
        this.specifiedZgramId = "";
        this.specifiedTimestamp = "";
        this.lastValidation = new AckSyntaxCheck("", true, "");
    }

    resetToIq(iq: InitialQuery) {
        class MyHandler {
            constructor(readonly owner: QueryViewModel) {}
            visitEnd() {
                this.owner.startAt = StartAt.End;
            }
            visitTimestamp(ts: number) {
                this.owner.startAt = StartAt.Timestamp;
                this.owner.specifiedTimestamp = ts.toString();
            }
            visitZgramId(zgId: ZgramId) {
                this.owner.startAt = StartAt.ZgramId;
                this.owner.specifiedZgramId = zgId.raw.toString();
            }
        }
        this.reset();
        this.query = iq.query;
        iq.searchOrigin.acceptVisitor(new MyHandler(this));
        this.onQueryChanged(this.query);
    }

    performQuery() {
        const sub = this.formatSubscribeMessage();
        if (sub === undefined) {
            // If we can't format a subcribe message, something is wrong with our parameters.
            return;
        }
        const query = new InitialQuery(sub.query, sub.searchOrigin);
        this.owner.openNewQuery(query);
    }

    get validity() {
        const sub = this.formatSubscribeMessage();
        if (sub === undefined) {
            // If we can't format a subcribe message, something is wrong with our parameters.
            return Validity.Invalid;
        }
        if (sub.query === this.lastValidation.query) {
            return this.lastValidation.valid ? Validity.Valid : Validity.Invalid;
        }
        return Validity.Unknown;
    }

    // Called back from the "watch" clause of the component
    onQueryChanged(newValue: string) {
        this.rateLimiter.invoke(this, () => {
            this.owner.performSyntaxCheck(newValue);
        });
    }

    onAckSyntaxCheck(ack: AckSyntaxCheck) {
        this.lastValidation = ack;
    }

    get valid() {
        return this.validity === Validity.Valid;
    }

    get invalid() {
        return this.validity === Validity.Invalid;
    }

    private formatSubscribeMessage() : QueryInfo | undefined {
        let searchOrigin: SearchOrigin;
        switch (this.startAt) {
            case StartAt.Begin: {
                searchOrigin = SearchOrigin.ofZgramId(new ZgramId(0));
                break;
            }

            case StartAt.End: {
                searchOrigin = SearchOrigin.ofEnd();
                break;
            }

            case StartAt.ZgramId: {
                const raw = parseInt(this.specifiedZgramId.trim());
                if (isNaN(raw)) {
                    return undefined;
                }
                searchOrigin = SearchOrigin.ofZgramId(new ZgramId(raw));
                break;
            }

            case StartAt.Timestamp: {
                const momentRes = moment.utc(this.specifiedTimestamp.trim());
                if (!momentRes.isValid()) {
                    return undefined;
                }
                const startTimestamp = momentRes.unix();
                searchOrigin = SearchOrigin.ofTimestamp(startTimestamp);
                break;
            }

            default: throw staticFail(this.startAt);
        }

        return new QueryInfo(this.query.trim(), searchOrigin);
    }
}

class QueryInfo {
    constructor(readonly query: string, readonly searchOrigin: SearchOrigin) {}
}
