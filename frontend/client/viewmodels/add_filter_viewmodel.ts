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

import {Filter} from "../../shared/protocol/misc";
import {Z2kState} from "../z2kstate";
import {staticFail} from "../../shared/utility";
import {ZgramViewModel} from "./zgram_viewmodel";

export class AddFilterViewModel {
    which: Which;
    duration: Duration;
    strong: boolean;

    constructor(readonly owner: ZgramViewModel, readonly state: Z2kState, readonly sender: string,
        readonly instance: string) {
        this.which = Which.Sender;
        this.duration = Duration.OneHour;
        this.strong = false;
    }

    get whichChoices() {
        return [
            new WhichChoice(Which.Sender, `sender "${this.sender}"`),
            new WhichChoice(Which.InstanceExact, `instance "${this.instance}"`),
            new WhichChoice(Which.InstancePrefix, `instance starts with "${this.instance}"`)
            ];
    }

    get whichDurations() {
        return [
            new WhichDuration(Duration.OneHour, "One Hour"),
            new WhichDuration(Duration.OneDay, "One Day"),
            new WhichDuration(Duration.OneWeek, "One Week")
        ];
    }

    get whichStrengths() {
        return [
            new WhichStrength(false, "Hide zgram body (show headers)"),
            new WhichStrength(true, "Completely hide zgram"),
        ];
    }

    addFilter() {
        const id = crypto.randomUUID();
        const [s, ix, ip] = this.calcFilterParams();
        const expiration = this.calcExpirationSecs();
        const filter = new Filter(id, s, ix, ip, this.strong, expiration);
        this.state.filtersViewModel.add(filter);
        this.owner.toggleAddFilterInteraction();
    }

    private calcExpirationSecs() {
        const oneHour = 60 * 60;
        const oneDay = oneHour * 24;
        const oneWeek = oneDay * 7;

        const nowSecs = Date.now() / 1000;
        switch (this.duration) {
            case Duration.OneHour: return nowSecs + oneHour;
            case Duration.OneDay: return nowSecs + oneDay;
            case Duration.OneWeek: return nowSecs + oneWeek;
            default: staticFail(this.duration);
        }
    }

    private calcFilterParams() {
        let sender: string | undefined = undefined;
        let instanceExact: string | undefined = undefined;
        let instancePrefix: string | undefined = undefined;

        switch (this.which) {
            case Which.Sender: sender = this.sender; break;
            case Which.InstanceExact: instanceExact = this.instance; break;
            case Which.InstancePrefix: instancePrefix = this.instance; break;
            default: staticFail(this.which);
        }

        return [sender, instanceExact, instancePrefix];
    }
}

enum Which {
    Sender = "Sender",
    InstanceExact = "InstanceExact",
    InstancePrefix = "InstancePrefix"
}

enum Duration {
    OneHour = "OneHour",
    OneDay = "OneDay",
    OneWeek = "OneWeek"
}

class WhichChoice {
    constructor(readonly tag: Which, readonly text: string) {}
}

class WhichDuration {
    constructor(readonly tag: Duration, readonly text: string) {}
}

class WhichStrength {
    constructor(readonly tag: boolean, readonly text: string) {}
}
