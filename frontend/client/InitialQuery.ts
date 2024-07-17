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

import {SearchOrigin, searchOriginInfo, ZgramId} from "../shared/protocol/zephyrgram";
import {Filter} from "../shared/protocol/misc"
import {assertArray, assertString} from "../shared/json_util";
import {escapeQuotes} from "../shared/utility";

enum QueryKeys {
    ZgramId = "id",
    Sender = "s",
    Instance = "i",
    General = "q",
}

enum SearchOriginKeys {
    ZgramId = "soid",
    Timestamp = "sots",
}

enum FilterKeys  {
    Filters = "fs"
}

export class InitialQuery {
    static ofDefault(searchOrigin: SearchOrigin, filters: Filter[]) {
        return new InitialQuery(searchOrigin, filters);
    }

    static ofGeneralQuery(query: string, searchOrigin: SearchOrigin, filters: Filter[]) {
        var result = new InitialQuery(searchOrigin, filters);
        result.query = query;
        return result;
    }

    static ofId(id: ZgramId, filters: Filter[]) {
        var result = new InitialQuery(SearchOrigin.ofEnd(), filters);
        result.zgramId = id.raw;
        return result;
    }

    static ofSender(sender: string, searchOrigin: SearchOrigin, filters: Filter[]) {
        var result = new InitialQuery(searchOrigin, filters);
        result.sender = sender;
        return result;
    }

    static ofInstance(instance: string, searchOrigin: SearchOrigin, filters: Filter[]) {
        var result = new InitialQuery(searchOrigin, filters);
        result.instance = instance;
        return result;
    }

    static createFromLocationOrDefault(location: Location) {
        var result = new InitialQuery(SearchOrigin.ofEnd(), []);
        const searchParams = new URLSearchParams(location.search);
        for (const [key, value] of searchParams) {
            switch (key) {
                case QueryKeys.ZgramId: {
                    result.zgramId = parseInt(value);
                    break;
                }
                case QueryKeys.Sender: {
                    result.sender = value;
                    break;
                }
                case QueryKeys.Instance: {
                    result.instance = value;
                    break;
                }
                case QueryKeys.General: {
                    result.query = value;
                    break
                }
                case SearchOriginKeys.ZgramId: {
                    const id = parseInt(value);
                    result.searchOrigin = SearchOrigin.ofZgramId(new ZgramId(id));
                    break;
                }
                case SearchOriginKeys.Timestamp: {
                    const ts = parseInt(value);
                    result.searchOrigin = SearchOrigin.ofTimestamp(ts);
                    break;
                }
                case FilterKeys.Filters: {
                    const object = JSON.parse(value);
                    const array = assertArray(object);
                    result.filters = array.map(Filter.tryParseJson);
                    break;
                }
                default: {
                    console.log(`Unrecognized key ${key}`);
                }
            }
        }
        return result;
    }

    private zgramId?: number = undefined;
    private sender?: string = undefined;
    private instance?: string = undefined;
    private query?: string = undefined;

    private constructor(private searchOrigin: SearchOrigin, private filters: Filter[]) {
    }

    toQueryString() {
        if (this.zgramId !== undefined) {
            return `zgramid(${this.zgramId}`;
        }

        if (this.sender !== undefined) {
            return `sender:${this.sender}`;
        }

        if (this.instance !== undefined) {
            const escaped = escapeQuotes(this.instance);
            return `instance:^literally("${escaped}")`;
        }

        return this.query === undefined ? "" : this.query;
    }

    toUrl(location: string) {
        const url = new URL(location);
        const sp = url.searchParams;
        if (this.zgramId !== undefined) {
            sp.append(QueryKeys.ZgramId, this.zgramId.toString());
        }
        if (this.sender !== undefined) {
            sp.append(QueryKeys.Sender, this.sender);
        }
        if (this.instance !== undefined) {
            sp.append(QueryKeys.Instance, this.instance);
        }
        if (this.query !== undefined && this.query !== "") {
            sp.append(QueryKeys.General, this.query);
        }
        if (this.searchOrigin.tag === searchOriginInfo.Tag.ZgramId) {
            sp.append(SearchOriginKeys.ZgramId, (this.searchOrigin.payload as ZgramId).raw.toString());
        }
        if (this.searchOrigin.tag === searchOriginInfo.Tag.Timestamp) {
            sp.append(SearchOriginKeys.ZgramId, (this.searchOrigin.payload as number).toString());
        }
        if (this.filters.length !== 0) {
            const asJson = JSON.stringify(this.filters.map(f => f.toJson()));
            url.searchParams.append(FilterKeys.Filters, asJson);
        }
        return url.toString();
    }

}
