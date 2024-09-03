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

import {Profile} from "../shared/protocol/profile";

export class SessionStatus {
    profile: Profile = Profile.empty();
    attachedToSession: boolean = false;
    haveRecentPing: boolean = false;
    queryOutstanding: boolean = false;
    getMoreZgramsRequestOutstanding: boolean = false;
    hasFilters: boolean = false;
    filtersEnabled: boolean = true;

    get isSessionHealthy() {
        return this.attachedToSession && this.haveRecentPing;
    }
}
