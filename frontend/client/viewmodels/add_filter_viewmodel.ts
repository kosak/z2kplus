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
    strong: boolean;
    senderFilter: WhichFilter;
    instanceFilter: WhichFilter;
    instancePrefixFilter: WhichFilter;
    allFilters: WhichFilter[];

    constructor(readonly owner: ZgramViewModel, readonly state: Z2kState, readonly sender: string,
        readonly instance: string) {
        this.strong = false;
        this.senderFilter = new WhichFilter(`sender "${this.sender}"`);
        this.instanceFilter = new WhichFilter(`instance "${this.instance}"`);
        this.instancePrefixFilter = new WhichFilter(`instance starts with "${this.instance}"`);
        this.allFilters = [this.senderFilter, this.instanceFilter, this.instancePrefixFilter];
    }

    get whichFilters() {
        return this.allFilters;
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
        const sender = this.senderFilter.valueIfSelected;
        const instanceExact = this.instanceFilter.valueIfSelected;
        const instancePrefix = this.instancePrefixFilter.valueIfSelected;
        return [sender, instanceExact, instancePrefix];
    }
}

class WhichFilter {
    selected: boolean;

    constructor(readonly text: string) {
        this.selected = false;
    }

    get valueIfSelected() {
        return this.selected ? this.text : undefined;
    }
}

class WhichStrength {
    constructor(readonly tag: boolean, readonly text: string) {}
}
