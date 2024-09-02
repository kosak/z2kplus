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
import {escapeQuotes} from "../shared/utility";

enum QueryKeys {
    ZgramId = "id",
    Sender = "s",
    Instance = "i",
    Reaction = "rx",
    General = "q",
}

enum SearchOriginKeys {
    ZgramId = "soid",
    Timestamp = "sots",
}

class Builder {
    zgramId: number | undefined = undefined;
    sender: string | undefined = undefined;
    instance: string | undefined = undefined;
    reaction: string | undefined = undefined;
    query: string | undefined = undefined;

    constructor(public searchOrigin: SearchOrigin) {
    }

    toInitialQuery() {
        return new InitialQuery(this.zgramId, this.sender, this.instance, this.reaction,
            this.query, this.searchOrigin);
    }
}


export class InitialQuery {
    static ofDefault(searchOrigin: SearchOrigin) {
        const builder = new Builder(searchOrigin);
        return builder.toInitialQuery();
    }

    static ofGeneralQuery(query: string, searchOrigin: SearchOrigin) {
        const builder = new Builder(searchOrigin);
        builder.query = query;
        return builder.toInitialQuery();
    }

    static ofId(id: ZgramId) {
        const builder = new Builder(SearchOrigin.ofEnd());
        builder.zgramId = id.raw;
        return builder.toInitialQuery();
    }

    static ofSender(sender: string, searchOrigin: SearchOrigin) {
        const builder = new Builder(searchOrigin);
        builder.sender = sender;
        return builder.toInitialQuery();
    }

    static ofInstance(instance: string, searchOrigin: SearchOrigin) {
        const builder = new Builder(searchOrigin);
        builder.instance = instance;
        return builder.toInitialQuery();
    }

    static ofReaction(reaction: string, searchOrigin: SearchOrigin) {
        const builder = new Builder(searchOrigin);
        builder.reaction = reaction;
        return builder.toInitialQuery();
    }

    static createFromLocationOrDefault(location: Location) {
        const builder = new Builder(SearchOrigin.ofEnd());
        const searchParams = new URLSearchParams(location.search);
        for (const [key, value] of searchParams) {
            switch (key) {
                case QueryKeys.ZgramId: {
                    builder.zgramId = parseInt(value);
                    break;
                }
                case QueryKeys.Sender: {
                    builder.sender = value;
                    break;
                }
                case QueryKeys.Instance: {
                    builder.instance = value;
                    break;
                }
                case QueryKeys.General: {
                    builder.query = value;
                    break
                }
                case SearchOriginKeys.ZgramId: {
                    const id = parseInt(value);
                    builder.searchOrigin = SearchOrigin.ofZgramId(new ZgramId(id));
                    break;
                }
                case SearchOriginKeys.Timestamp: {
                    const ts = parseInt(value);
                    builder.searchOrigin = SearchOrigin.ofTimestamp(ts);
                    break;
                }
                default: {
                    console.log(`Unrecognized key ${key}`);
                }
            }
        }
        return builder.toInitialQuery();
    }

    public constructor(private readonly zgramId: number | undefined,
        private readonly sender: string | undefined,
        private readonly instance: string | undefined,
        private readonly reaction: string | undefined,
        private readonly query: string | undefined,
        public readonly searchOrigin: SearchOrigin) {
    }

    toQueryString() {
        if (this.zgramId !== undefined) {
            return `zgramid(${this.zgramId})`;
        }

        if (this.sender !== undefined) {
            return `sender:${this.sender}`;
        }

        if (this.instance !== undefined) {
            const escaped = escapeQuotes(this.instance);
            return `instance:^literally("${escaped}")`;
        }

        if (this.reaction !== undefined) {
            const escaped = escapeQuotes(this.reaction);
            return `hasreaction("${escaped}")`;
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
        if (this.reaction !== undefined) {
            sp.append(QueryKeys.Reaction, this.reaction);
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
        return url.toString();
    }
}
