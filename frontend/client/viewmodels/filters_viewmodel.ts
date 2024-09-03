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

export class FiltersViewModel {
    private filters: Filter[];

    constructor(readonly state: Z2kState) {
        this.filters = [];
    }

    reset(filters: Filter[]) {
        this.filters = filters;
    }

    matchesAny(strongOnly: boolean, sender: string, instance: string) {
        // Because there are so few filters, we don't bother to be efficient
        for (const filter of this.filters) {
            if (matches(filter, sender, instance, strongOnly)) {
                return true;
            }
        }
        return false;
    }

    get allFilterViewModels() {
        return this.allFilters.map(f => new FilterViewModel(this, this.state, f));
    }

    get allFilters() {
        return this.filters;
    }
}

function matches(filter: Filter, sender: string, instance: string, strongOnly: boolean) {
    if (strongOnly && !filter.strong) {
        return false;
    }
    if (filter.sender !== undefined && filter.sender !== sender) {
        return false;
    }
    if (filter.instanceExact !== undefined && filter.instanceExact !== instance) {
        return false;
    }
    if (filter.instancePrefix !== undefined && !instance.startsWith(filter.instancePrefix)) {
        return false;
    }
    return true;
}

export class FilterViewModel {
    constructor(readonly owner: FiltersViewModel, readonly state: Z2kState, readonly filter: Filter) {}

    get sender() {
        return this.filter.sender;
    }

    get instanceExact() {
        return this.filter.instanceExact;
    }

    get instancePrefix() {
        return this.filter.instancePrefix;
    }

    get strong() {
        return this.filter.strong;
    }

    get humanReadableDescription() {
        const items: string[] = [];
        if (this.sender !== undefined) {
            items.push(`sender:"${this.sender}"`);
        }
        if (this.instanceExact !== undefined) {
            items.push(`instance:"${this.instanceExact}"`);
        }
        if (this.instancePrefix !== undefined) {
            items.push(`instance starts with:"${this.instancePrefix}"`);
        }
        if (items.length === 0) {
            return "(match all)";
        }
        const joined = items.join(",");
        const weakOrStrong = this.strong ? "strong" : "weak";
        return `${joined} [${weakOrStrong}]`;
    }

    removeSelf() {
        this.state.removeFilter(this.filter);
    }
}
