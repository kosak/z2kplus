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

import {MetadataRecord, SearchOrigin, ZgramCore, ZgramId} from "../zephyrgram";
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
import {makeCommaSeparatedList, Optional, Pair, staticFail} from "../../utility";

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

  export class PostZgrams {
    constructor(readonly zgramsAndRefersTo: Pair<ZgramCore, Optional<ZgramId>>[]) {
    }

    toJson() {
      const zgJson = this.zgramsAndRefersTo.map(zgPair => zgPair.toJson());
      return [zgJson];
    }

    static tryParseJson(item: any) {
      const array = assertAndDestructure1(item, assertArray);
      const pairs = array.map(
          pair => Pair.tryParseJson(
              pair,
              first => ZgramCore.tryParseJson(first),
              second => Optional.tryParseJson(second, ZgramId.tryParseJson))
      );
      return new PostZgrams(pairs);
    }

    toString() {
      return `Post(${makeCommaSeparatedList(this.zgramsAndRefersTo)})`;
    }
  }

  export class GetSpecificZgrams {
    constructor(readonly zgramIds: ZgramId[]) {
    }

    toJson() {
      const zgJson = this.zgramIds.map(zgId => zgId.toJson());
      return [zgJson];
    }

    static tryParseJson(item: any) {
      const array = assertAndDestructure1(item, assertArray);
      const zgIds = array.map(ZgramId.tryParseJson);
      return new GetSpecificZgrams(zgIds);
    }

    toString() {
      return `Post(${makeCommaSeparatedList(this.zgramIds)})`;
    }
  }

  export class PostMetadata {
    constructor(readonly metadata: MetadataRecord[]) {
    }

    toJson() {
      const mdJson = this.metadata.map(o => o.toJson());
      return [mdJson];
    }

    static tryParseJson(item: any) {
      const [mdArray] = assertAndDestructure1(item, assertArray);
      const mds = mdArray.map(MetadataRecord.tryParseJson);
      return new PostMetadata(mds);
    }

    toString() {
      return `Post(${makeCommaSeparatedList(this.metadata)})`;
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
    PostZgrams = "PostZgrams",
    // Get specific zgrams that you happen to care about (e.g. those that appear in a refers-to)
    GetSpecificZgrams = "GetSpecificZgrams",
    // Post new metadata
    PostMetadata = "PostMetadata",
    // For debugging. A round-trip to the server. When you get the Ack back, you know that the
    // server has processed all your requests up through the Ack.
    Ping = "Ping",
  }

  export type payload_t = drequests.CheckSyntax | drequests.Subscribe |
      drequests.GetMoreZgrams | drequests.PostZgrams | drequests.PostMetadata |
      drequests.GetSpecificZgrams | drequests.Ping;

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

  export interface IPostZgrams {
    readonly tag: Tag.PostZgrams;
    readonly payload: drequests.PostZgrams;
  }

  export interface IGetSpecificZgrams {
    readonly tag: Tag.GetSpecificZgrams;
    readonly payload: drequests.GetSpecificZgrams;
  }

  export interface IPostMetadata {
    readonly tag: Tag.PostMetadata;
    readonly payload: drequests.PostMetadata;
  }

  export interface IPing {
    readonly tag: Tag.Ping;
    readonly payload: drequests.Ping;
  }

  export interface IVisitor<TResult> {
    visitCheckSyntax(o: drequests.CheckSyntax): TResult;

    visitSubscribe(o: drequests.Subscribe): TResult;

    visitGetMoreZgrams(o: drequests.GetMoreZgrams): TResult;

    visitPostZgrams(o: drequests.PostZgrams): TResult;

    visitGetSpecificZgrams(o: drequests.GetSpecificZgrams): TResult;

    visitPostMetadata(o: drequests.PostMetadata): TResult;

    visitPing(o: drequests.Ping): TResult;
  }
}  // namespace drequestInfo

export type IDRequest = drequestInfo.ICheckSyntax | drequestInfo.ISubscribe |
    drequestInfo.IGetMoreZgrams | drequestInfo.IPostZgrams | drequestInfo.IGetSpecificZgrams |
    drequestInfo.IPostMetadata | drequestInfo.IPing;

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
  static createPostZgrams(zgPairs: Pair<ZgramCore, Optional<ZgramId>>[]) {
    return new DRequest(drequestInfo.Tag.PostZgrams, new drequests.PostZgrams(zgPairs));
  }
  static createPostMetadata(metadata: MetadataRecord[]) {
    return new DRequest(drequestInfo.Tag.PostMetadata, new drequests.PostMetadata(metadata));
  }
  static createGetSpecificZgrams(zgramIds: ZgramId[]) {
    return new DRequest(drequestInfo.Tag.GetSpecificZgrams, new drequests.GetSpecificZgrams(zgramIds));
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
      case drequestInfo.Tag.PostZgrams: return visitor.visitPostZgrams(idreq.payload);
      case drequestInfo.Tag.PostMetadata: return visitor.visitPostMetadata(idreq.payload);
      case drequestInfo.Tag.GetSpecificZgrams: return visitor.visitGetSpecificZgrams(idreq.payload);
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
      case drequestInfo.Tag.PostZgrams: payload = drequests.PostZgrams.tryParseJson(item1); break;
      case drequestInfo.Tag.PostMetadata: payload = drequests.PostMetadata.tryParseJson(item1); break;
      case drequestInfo.Tag.GetSpecificZgrams: payload = drequests.GetSpecificZgrams.tryParseJson(item1); break;
      case drequestInfo.Tag.Ping: payload = drequests.Ping.tryParseJson(item1); break;
      default: throw staticFail(tag);
    }
    return new DRequest(tag, payload);
  }
}
