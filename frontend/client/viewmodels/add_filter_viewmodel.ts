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
    strong: boolean;

    constructor(readonly owner: ZgramViewModel, readonly state: Z2kState, readonly sender: string,
        readonly instance: string) {
        this.which = Which.Sender;
        this.strong = false;
    }

    get whichChoices() {
        return [
            new WhichChoice(Which.Sender, `sender "${this.sender}"`),
            new WhichChoice(Which.InstanceExact, `instance "${this.instance}"`),
            new WhichChoice(Which.InstancePrefix, `instance starts with "${this.instance}"`)
            ];
    }

    get whichStrengths() {
        return [
            new WhichStrength(false, "Hide zgram body (show headers)"),
            new WhichStrength(true, "Completely hide zgram"),
        ];
    }

    addFilter() {
        const [s, ix, ip] = this.calcFilterParams();
        const filter = new Filter(s, ix, ip, this.strong);
        this.state.addFilter(filter);
        this.owner.toggleAddFilterInteraction();
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

class WhichChoice {
    constructor(readonly tag: Which, readonly text: string) {}
}

class WhichStrength {
    constructor(readonly tag: boolean, readonly text: string) {}
}
