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

import {SearchOrigin} from "../shared/protocol/zephyrgram";
import {Filter} from "../shared/protocol/misc"
import {assertAndDestructure3, assertArray, assertArrayOfLength, assertString} from "../shared/json_util";

export class InitialQuery {
    static createFromLocationOrDefault(location: Location) {
        const defaultQuery = new InitialQuery("", SearchOrigin.ofEnd(), []);
        const search = new URLSearchParams(location.search.substring(1));
        const query = search.get("query");
        if (!query) {
            return defaultQuery;
        }
        try {
            const json = JSON.parse(query);
            return InitialQuery.tryParseJson(json);
        } catch (e) {
            console.log(`Couldn't parse params from ${location}`);
            return defaultQuery;
        }
    }

    constructor(readonly query: string, readonly searchOrigin: SearchOrigin, readonly filters: Filter[]) {
    }

    toJson() {
        return [this.query, this.searchOrigin.toJson(), this.filters.map(f => f.toJson())];
    }

    static tryParseJson(item: any) {
        const [q, so, fs] = assertAndDestructure3(item, assertString, SearchOrigin.tryParseJson, assertArray);
        const filters = fs.map(Filter.tryParseJson);
        return new InitialQuery(q, so, filters);
    }
}
