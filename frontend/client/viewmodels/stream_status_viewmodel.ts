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
import {Estimate} from "../../shared/protocol/misc";

export class StreamStatusViewModel {
    estimate: Estimate;
    // Undefined means infinite appetite
    private appetite: number | undefined;
    wantSpeech: boolean;

    constructor(public readonly uniqueId: string, private readonly state: Z2kState,
        private readonly backSide: boolean) {
        this.estimate = new Estimate(0, true);
        this.appetite = 0;
        this.wantSpeech = false;
    }

    get humanReadableEstimateText() {
        const direction = this.backSide ? "later" : "earlier";
        let howmany: string;
        const e = this.estimate;
        if (e.exact && e.count === 0) {
            howmany = "no";
            if (this.backSide && this.appetite === undefined) {
                // Special case for backside.
                return "Waiting for more zgrams.";
            }
        } else {
            const optionalAtLeast = e.exact ? "" : "at least ";
            howmany = `${optionalAtLeast}${this.estimate.count}`;
        }
        return `There are ${howmany} ${direction} zgrams.`;
    }

    get buttons() {
        if (this.appetite === undefined || this.appetite > 0) {
            // No need for "Get More" buttons if there is an appetite for more zgrams.
            return [];
        }
        const e = this.estimate;
        const callback = (self: ButtonViewModel) => {
            this.appetite = self.count;
            this.state.sendMoreIfThereIsSufficientAppetite();
        };
        if (e.exact && e.count === 0) {
            // When the server reports exactly zero zgrams...
            // For the front side, this should be interpreted as "No more zgrams ... ever".
            // But for the back side, this should be interpreted as "No more zgrams ... at the moment".
            // For the back side, we give people a button that offers them to start streaming.
            if (this.backSide) {
                const button = new ButtonViewModel(undefined, callback);
                return [button];
            }
        }
        const buttons: ButtonViewModel[] = [];
        const sizes = [10, 100, 500, 1000];
        if (e.exact) {
            sizes.push(e.count);
        }
        sizes.sort((l, r) => l - r);
        for (const size of sizes) {
            if (!e.exact || e.count >= size) {
                buttons.push(new ButtonViewModel(size, callback));
            }
        }
        return buttons;
    }

    calcGetMoreAmount() {
        const e = this.estimate;
        const roughlyAvailable = e.exact ? e.count : Number.MAX_SAFE_INTEGER;
        const roughlyDesired = this.appetite !== undefined ? this.appetite : Number.MAX_SAFE_INTEGER;
        return Math.min(roughlyAvailable, roughlyDesired);
    }

    setAppetite(newAppetite: number | undefined) {
        this.appetite = newAppetite;
    }

    reduceAppetite(amount: number) {
        if (this.appetite === undefined) {
            return;
        }
        this.appetite -= amount;
    }

    get isStreaming() {
        return this.appetite === undefined;
    }
}

class ButtonViewModel {
    constructor(readonly count: number | undefined, private readonly onClick: (self: ButtonViewModel) => void) {}

    doClick() {
        this.onClick(this);
    }

    get text() {
        if (this.count === undefined) {
            return "Stream";
        }
        return `Get ${this.count} more`;
    }
}
