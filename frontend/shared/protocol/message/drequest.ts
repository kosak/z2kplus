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

import {MetadataRecord, SearchOrigin, ZgramCore} from "../zephyrgram";
import {
  assertAndDestructure1,
  assertAndDestructure2,
  assertAndDestructure4,
  assertArray,
  assertBoolean,
  assertEnum,
  assertNumber,
  assertString
}
  from "../../json_util";
import {makeCommaSeparatedList, staticFail} from "../../utility";

export namespace drequests {
  export class CheckSyntax {
    constructor(readonly query: string) {
    }

    toJson() {
      return [this.query];
    }

    static tryParseJson(item: any) {
      const [query] = assertAndDestructure1(item, assertString);
      return new CheckSyntax(query);
    }

    toString() {
      return `CheckSyntax(${this.query})`;
    }
  }

  export class Subscribe {
    constructor(readonly query: string, readonly searchOrigin: SearchOrigin, readonly pageSize: number,
        readonly queryMargin: number) {
    }

    toJson() {
      return [this.query, this.searchOrigin.toJson(), this.pageSize, this.queryMargin];
    }

    static tryParseJson(item: any) {
      const [q, so, ps, qm] = assertAndDestructure4(item, assertString, SearchOrigin.tryParseJson,
          assertNumber, assertNumber);
      return new Subscribe(q, so, ps, qm);
    }

    toString() {
      return `Subscribe(${this.query}, ${this.searchOrigin}, ${this.pageSize}, ${this.queryMargin}`;
    }
  }

  export class GetMoreZgrams {
    constructor(public readonly forBackSide: boolean, public readonly count: number) {
    }

    toJson() {
      return [this.forBackSide, this.count];
    }

    static tryParseJson(item: any) {
      const [bs, c] = assertAndDestructure2(item, assertBoolean, assertNumber);
      return new GetMoreZgrams(bs, c);
    }

    toString() {
      return `GetMoreZgrams(${this.forBackSide}, ${this.count})`;
    }
  }

  export class Post {
    constructor(readonly zgrams: ZgramCore[], readonly metadata: MetadataRecord[]) {
    }

    toJson() {
      const zgJson = this.zgrams.map(o => o.toJson());
      const mdJson = this.metadata.map(o => o.toJson());
      return [zgJson, mdJson];
    }

    static tryParseJson(item: any) {
      const [zgArray, mdArray] = assertAndDestructure2(item, assertArray, assertArray);
      const zgs = zgArray.map(ZgramCore.tryParseJson);
      const mds = mdArray.map(MetadataRecord.tryParseJson);
      return new Post(zgs, mds);
    }

    toString() {
      return `Post(${makeCommaSeparatedList(this.zgrams)}, ${makeCommaSeparatedList(this.metadata)})`;
    }
  }

  export class Ping {
    constructor(readonly cookie: number) {
    }

    toJson() {
      return [this.cookie];
    }

    static tryParseJson(item: any) {
      const [cookie] = assertAndDestructure1(item, assertNumber);
      return new Ping(cookie);
    }

    toString() {
      return `Ping(${this.cookie})`;
    }
  }
}

namespace drequestInfo {
  export const enum Tag {
    // Check the syntax of a query
    CheckSyntax = "CheckSyntax",
    // Subscribe to or unsubscribe from a stream of Zgrams
    Subscribe = "Subscribe",
    // Request (up to) N more zgrams from the front or back side.
    GetMoreZgrams = "GetMoreZgrams",
    // Post new zgrams
    Post = "Post",
    // For debugging. A round-trip to the server. When you get the Ack back, you know that the
    // server has processed all your requests up through the Ack.
    Ping = "Ping",
  }

  export type payload_t = drequests.CheckSyntax | drequests.Subscribe |
      drequests.GetMoreZgrams | drequests.Post | drequests.Ping;

  export interface ICheckSyntax {
    readonly tag: Tag.CheckSyntax;
    readonly payload: drequests.CheckSyntax;
  }

  export interface ISubscribe {
    readonly tag: Tag.Subscribe;
    readonly payload: drequests.Subscribe;
  }

  export interface IGetMoreZgrams {
    readonly tag: Tag.GetMoreZgrams;
    readonly payload: drequests.GetMoreZgrams;
  }

  export interface IPost {
    readonly tag: Tag.Post;
    readonly payload: drequests.Post;
  }

  export interface IPing {
    readonly tag: Tag.Ping;
    readonly payload: drequests.Ping;
  }

  export interface IVisitor<TResult> {
    visitCheckSyntax(o: drequests.CheckSyntax): TResult;

    visitSubscribe(o: drequests.Subscribe): TResult;

    visitGetMoreZgrams(o: drequests.GetMoreZgrams): TResult;

    visitPost(o: drequests.Post): TResult;

    visitPing(o: drequests.Ping): TResult;
  }
}  // namespace drequestInfo

export type IDRequest = drequestInfo.ICheckSyntax | drequestInfo.ISubscribe |
    drequestInfo.IGetMoreZgrams | drequestInfo.IPost | drequestInfo.IPing;

export class DRequest {
  static createCheckSyntax(query: string) {
    const payload = new drequests.CheckSyntax(query);
    return new DRequest(drequestInfo.Tag.CheckSyntax, payload);
  }
  static createSubscribe(query: string, searchOrigin: SearchOrigin, pageSize: number, queryMargin: number) {
    const payload = new drequests.Subscribe(query, searchOrigin, pageSize, queryMargin);
    return new DRequest(drequestInfo.Tag.Subscribe, payload);
  }
  static createGetMoreZgrams(forBackSide: boolean, count: number) {
    return new DRequest(drequestInfo.Tag.GetMoreZgrams, new drequests.GetMoreZgrams(forBackSide, count));
  }
  static createPost(zgs: ZgramCore[], mds: MetadataRecord[]) {
    return new DRequest(drequestInfo.Tag.Post, new drequests.Post(zgs, mds));
  }
  static createPing(cookie: number) {
    return new DRequest(drequestInfo.Tag.Ping, new drequests.Ping(cookie));
  }

  constructor(readonly tag : drequestInfo.Tag, readonly payload: drequestInfo.payload_t) {
  }
  
  acceptVisitor<TResult>(visitor: drequestInfo.IVisitor<TResult>) : TResult {
    const idreq = this as IDRequest;
    switch (idreq.tag) {
      case drequestInfo.Tag.CheckSyntax: return visitor.visitCheckSyntax(idreq.payload);
      case drequestInfo.Tag.Subscribe: return visitor.visitSubscribe(idreq.payload);
      case drequestInfo.Tag.GetMoreZgrams: return visitor.visitGetMoreZgrams(idreq.payload);
      case drequestInfo.Tag.Post: return visitor.visitPost(idreq.payload);
      case drequestInfo.Tag.Ping: return visitor.visitPing(idreq.payload);
      default: staticFail(idreq);
    }
    throw new Error("notreached");
  }

  toJson() {
    const variant = [this.tag, this.payload.toJson()];
    return [variant];
  }

  toString() {
    return `Request(${this.tag}, ${this.payload})`;
  }

  static tryParseJson(item: any) {
    const [variant] = assertAndDestructure1(item, assertArray);
    const [tag, item1] = assertAndDestructure2(variant, i => assertEnum<drequestInfo.Tag>(i), i => i);
    let payload: drequestInfo.payload_t;
    switch (tag) {
      case drequestInfo.Tag.CheckSyntax: payload = drequests.CheckSyntax.tryParseJson(item1); break;
      case drequestInfo.Tag.Subscribe: payload = drequests.Subscribe.tryParseJson(item1); break;
      case drequestInfo.Tag.GetMoreZgrams: payload = drequests.GetMoreZgrams.tryParseJson(item1); break;
      case drequestInfo.Tag.Post: payload = drequests.Post.tryParseJson(item1); break;
      case drequestInfo.Tag.Ping: payload = drequests.Ping.tryParseJson(item1); break;
      default: throw staticFail(tag);
    }
    return new DRequest(tag, payload);
  }
}
