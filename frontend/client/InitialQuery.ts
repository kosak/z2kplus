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

import {SearchOrigin, searchOriginInfo} from "../shared/protocol/zephyrgram";
import {Filter} from "../shared/protocol/misc"
import {assertAndDestructure3, assertArray, assertArrayOfLength, assertString} from "../shared/json_util";


enum QueryKeys {
    Query = "q",
    SearchOrigin = "so",
    Filters = "fs"
}

export class InitialQuery {
    static createFromLocationOrDefault(location: Location) {
        let query = "";
        let searchOrigin = SearchOrigin.ofEnd();
        let filters: Filter[] = [];

        const searchParams = new URLSearchParams(location.search);
        for (const [key, value] of searchParams) {
            const valueAsObject = JSON.parse(value);
            switch (key) {
                case QueryKeys.Query: {
                    query = assertString(valueAsObject);
                    break;
                }
                case QueryKeys.SearchOrigin: {
                    searchOrigin = SearchOrigin.tryParseJson(valueAsObject);
                    break;
                }
                case QueryKeys.Filters: {
                    const fs = assertArray(valueAsObject);
                    filters = fs.map(Filter.tryParseJson);
                    break
                }
                default: {
                    console.log(`Unrecognized key ${key}`);
                }
            }
        }
        return new InitialQuery(query, searchOrigin, filters);
    }

    toUrl(location: string) {
        const url = new URL(location);
        if (this.query !== "") {
            url.searchParams.append(QueryKeys.Query, this.query);
        }
        if (this.searchOrigin.tag !== searchOriginInfo.Tag.End) {
            const asJson = JSON.stringify(this.searchOrigin.toJson());
            url.searchParams.append(QueryKeys.SearchOrigin, asJson);
        }
        if (this.filters.length !== 0) {
            const asJson = JSON.stringify(this.filters.map(f => f.toJson()));
            url.searchParams.append(QueryKeys.Filters, asJson);
        }
        return url.toString();
    }

    constructor(readonly query: string, readonly searchOrigin: SearchOrigin, readonly filters: Filter[]) {
    }
}
