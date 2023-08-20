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

import {
  anyToJson,
  assertAndDestructure1,
  assertAndDestructure2,
  assertAndDestructure3,
  assertAndDestructure4,
  assertAndDestructure6,
  assertArray,
  assertBoolean,
  assertEnum,
  assertNumber,
  assertString
} from "../json_util";
import {staticFail, tryParseWholeNumber} from "../utility";
import {Unit} from "./misc";

export class ZgramId {
  constructor(public readonly raw: number) {}

  toJson() {
    return [this.raw];
  }

  static tryParseJson(src: any) {
    const [raw] = assertAndDestructure1(src, assertNumber);
    return new ZgramId(raw);
  }

  compareTo(other: ZgramId) {
    return this.raw - other.raw;
  }

  toString() {
    return `ZgramId(${this.raw})`;
  }
}

export namespace searchOriginInfo {
  export const enum Tag {
    // start at end
    End = "end",
    // start at timestamp
    Timestamp = "ts",
    // start at zgramId
    ZgramId = "zg"
  }

  export type payload_t = Unit | number | ZgramId;

  export interface IEnd {
    readonly tag: Tag.End;
    readonly payload: Unit;
  }

  export interface ITimestmap {
    readonly tag: Tag.Timestamp;
    readonly payload: number;
  }

  export interface IZgramId {
    readonly tag: Tag.ZgramId;
    readonly payload: ZgramId;
  }

  export interface IVisitor<TResult> {
    visitEnd(o: Unit): TResult;

    visitTimestamp(o: number): TResult;

    visitZgramId(o: ZgramId): TResult;
  }
}

export type ISearchOrigin =
    searchOriginInfo.IEnd |
    searchOriginInfo.ITimestmap |
    searchOriginInfo.IZgramId;

export class SearchOrigin {
  static ofEnd() {
    return new SearchOrigin(searchOriginInfo.Tag.End, Unit.create());
  }

  static ofTimestamp(timestamp: number) {
    return new SearchOrigin(searchOriginInfo.Tag.Timestamp, timestamp);
  }

  static ofZgramId(zgramId: ZgramId) {
    return new SearchOrigin(searchOriginInfo.Tag.ZgramId, zgramId);
  }

  private constructor(readonly tag: searchOriginInfo.Tag, readonly payload: searchOriginInfo.payload_t) {
  }

  toJson() {
    const variant = [this.tag, anyToJson(this.payload)];
    return [variant];
  }

  acceptVisitor<TResult>(visitor: searchOriginInfo.IVisitor<TResult>) : TResult {
    const idresp = this as ISearchOrigin;
    switch (idresp.tag) {
      case searchOriginInfo.Tag.End:
        return visitor.visitEnd(idresp.payload);
      case searchOriginInfo.Tag.Timestamp:
        return visitor.visitTimestamp(idresp.payload);
      case searchOriginInfo.Tag.ZgramId:
        return visitor.visitZgramId(idresp.payload);
      default:
        staticFail(idresp);
    }
  }

  static tryParseJson(item: any) {
    const [variant] = assertAndDestructure1(item, assertArray);
    const [tag, item1] = assertAndDestructure2(variant, i => assertEnum<searchOriginInfo.Tag>(i), i => i);
    let payload: searchOriginInfo.payload_t;
    switch (tag) {
      case searchOriginInfo.Tag.End: payload = Unit.tryParseJson(item1); break;
      case searchOriginInfo.Tag.Timestamp: payload = assertNumber(item1); break;
      case searchOriginInfo.Tag.ZgramId: payload = ZgramId.tryParseJson(item1); break;
      default: throw staticFail(tag);
    }
    return new SearchOrigin(tag, payload);
  }

  toString() {
    return `Subscribe(${this.tag}, ${this.payload})`;
  }
}

export enum RenderStyle {Default = "d", MarkDeepMathAjax = "x"}

export class ZgramCore {
  // instance: The Zephyr instance. e.g. "help.cheese"
  // body: The body of the message. e.g. "Where can I find some good cheese?"
  constructor(readonly instance: string, readonly body: string, readonly renderStyle: RenderStyle) {
  }

  toJson() {
    return [this.instance, this.body, this.renderStyle];
  }

  static tryParseJson(src: object) {
    const [i, b, s] = assertAndDestructure3(src, assertString, assertString, i => assertEnum<RenderStyle>(i));
    return new ZgramCore(i, b, s);
  }

  toString() {
    return `ZgramCore(${this.instance}, ${this.body}, ${this.renderStyle})`;
  }
}

export class Zephyrgram {
  // sender: e.g. "kosak" or "kosak.z"
  // signature: e.g. "Corey Kosak" or custom signature like "Lord Cinnabon".
  // isLogged: If the message is logged (vs graffiti)
  constructor(readonly zgramId: ZgramId, readonly timesecs: number, readonly sender: string,
      readonly signature: string, readonly isLogged: boolean, readonly zgramCore: ZgramCore) {
  }

  toJson() {
    return [this.zgramId, this.timesecs, this.sender, this.signature, this.isLogged, this.zgramCore.toJson() ];
  }

  static tryParseJson(src: any) {
    const [zgId, ts, s, sg, l, zgc] = assertAndDestructure6(src, ZgramId.tryParseJson, assertNumber,
      assertString, assertString, assertBoolean, ZgramCore.tryParseJson);
    return new Zephyrgram(zgId, ts, s, sg, l, zgc);
  }

  toString() {
    return `Zephyrgram(${this.zgramId}, ${this.timesecs}, ${this.zgramCore})`;
  }
}

export namespace zgMetadata {
  export class Reaction {
    constructor(readonly zgramId: ZgramId, readonly reaction: string,
                readonly creator: string, readonly value: boolean) {}

    toJson() {
      return [this.zgramId.toJson(), this.reaction, this.creator, this.value];
    }

    static tryParseJson(src: object) {
      const [zg, rx, c, v] = assertAndDestructure4(src, ZgramId.tryParseJson, assertString,
          assertString, assertBoolean);
      return new Reaction(zg, rx, c, v);
    }

    toString() {
      return `Reaction(${this.zgramId}, ${this.reaction}, ${this.creator}, ${this.value})`;
    }
  }

  export class ZgramRevision {
    constructor(readonly zgramId: ZgramId, readonly zgc: ZgramCore) {}

    toJson() {
      return [this.zgramId.toJson(), this.zgc.toJson()];
    }

    static tryParseJson(src: object) {
      const [id, zgc] = assertAndDestructure2(src, ZgramId.tryParseJson, ZgramCore.tryParseJson);
      return new ZgramRevision(id, zgc);
    }

    toString() {
      return `ZgramRevision(${this.zgramId}, ${this.zgc})`;
    }
  }

  export class ZgramRefersTo {
    constructor(readonly zgramId: ZgramId, readonly refersTo: ZgramId, readonly value: boolean) {}

    toJson() {
      return [this.zgramId.toJson(), this.refersTo.toJson(), this.value];
    }

    static tryParseJson(src: object) {
      const [id, rt, v] = assertAndDestructure3(src, ZgramId.tryParseJson, ZgramId.tryParseJson, assertBoolean);
      return new ZgramRefersTo(id, rt, v);
    }

    toString() {
      return `ZgramRefersTo(${this.zgramId}, ${this.refersTo}, ${this.value})`;
    }
  }
}  // namespace zgMetadata

export namespace userMetadata {
  export class Zmojis {
    constructor(readonly userId: string, readonly zmojis: string) {}

    toJson() {
      return [this.userId, this.zmojis];
    }

    static tryParseJson(src: object) {
      const [u, z] = assertAndDestructure2(src, assertString, assertString);
      return new Zmojis(u, z);
    }

    toString() {
      return `Zmojis(${this.userId}, ${this.zmojis})`;
    }
  }
}  // namespace zgMetadata

export namespace metadataRecordInfo {
  export const enum Tag {
    Reaction = "rx",
    ZgramRevision = "zgrev",
    ZgramRefersTo = "ref",
    Zmojis = "zmojis"
  }

  export type payload_t =
      zgMetadata.Reaction |
      zgMetadata.ZgramRevision |
      zgMetadata.ZgramRefersTo |
      userMetadata.Zmojis;

  // This drama is to support the discriminated union pattern.
  export interface IReaction {
    readonly tag: Tag.Reaction;
    readonly payload: zgMetadata.Reaction;
  }
  export interface IZgramRevision {
    readonly tag: Tag.ZgramRevision;
    readonly payload: zgMetadata.ZgramRevision;
  }
  export interface IZgramRefersTo {
    readonly tag: Tag.ZgramRefersTo;
    readonly payload: zgMetadata.ZgramRefersTo;
  }
  export interface IZmojis {
     readonly tag: Tag.Zmojis;
     readonly payload: userMetadata.Zmojis;
  }

  export interface IVisitor<TResult> {
    visitReaction(payload: zgMetadata.Reaction): TResult;

    visitZgramRevision(payload: zgMetadata.ZgramRevision): TResult;

    visitZgramRefersTo(payload: zgMetadata.ZgramRefersTo): TResult;

    visitZmojis(payload: userMetadata.Zmojis): TResult;
  }
}

export type IMetadataRecord =
    metadataRecordInfo.IReaction |
    metadataRecordInfo.IZgramRevision |
    metadataRecordInfo.IZgramRefersTo |
    metadataRecordInfo.IZmojis;

export class MetadataRecord {
  static createReaction(zgramId: ZgramId, reaction: string, creator: string, value: boolean) {
    const payload = new zgMetadata.Reaction(zgramId, reaction, creator, value);
    return new MetadataRecord(metadataRecordInfo.Tag.Reaction, payload);
  }

  static createZgramRevision(zgramId: ZgramId, zgc: ZgramCore) {
    const payload = new zgMetadata.ZgramRevision(zgramId, zgc);
    return new MetadataRecord(metadataRecordInfo.Tag.ZgramRevision, payload);
  }

  static createZgramRefersTo(zgramId: ZgramId, refersTo: ZgramId, value: boolean) {
    const payload = new zgMetadata.ZgramRefersTo(zgramId, refersTo, value);
    return new MetadataRecord(metadataRecordInfo.Tag.ZgramRefersTo, payload);
  }

  static createZmojis(userId: string, zmojis: string) {
    const payload = new userMetadata.Zmojis(userId, zmojis);
    return new MetadataRecord(metadataRecordInfo.Tag.Zmojis, payload);
  }

  constructor(readonly tag: metadataRecordInfo.Tag,
              readonly payload: metadataRecordInfo.payload_t) {}

  acceptVisitor<TResult>(visitor: metadataRecordInfo.IVisitor<TResult>) : TResult {
    const idresp = this as IMetadataRecord;
    switch (idresp.tag) {
      case metadataRecordInfo.Tag.Reaction:
        return visitor.visitReaction(idresp.payload);
      case metadataRecordInfo.Tag.ZgramRevision:
        return visitor.visitZgramRevision(idresp.payload);
      case metadataRecordInfo.Tag.ZgramRefersTo:
        return visitor.visitZgramRefersTo(idresp.payload);
      case metadataRecordInfo.Tag.Zmojis:
        return visitor.visitZmojis(idresp.payload);
      default:
        staticFail(idresp);
    }
  }

  toJson() {
    const variant = [this.tag, this.payload.toJson()];
    return [variant];
  }

  static tryParseJson(src: object) {
    const [variant] = assertAndDestructure1(src, x => x);
    const [tag, item] = assertAndDestructure2(variant, i => assertEnum<metadataRecordInfo.Tag>(i), x => x);
    switch (tag) {
      case metadataRecordInfo.Tag.Reaction:
        return new MetadataRecord(tag, zgMetadata.Reaction.tryParseJson(item));
      case metadataRecordInfo.Tag.ZgramRevision:
        return new MetadataRecord(tag, zgMetadata.ZgramRevision.tryParseJson(item));
      case metadataRecordInfo.Tag.ZgramRefersTo:
        return new MetadataRecord(tag, zgMetadata.ZgramRefersTo.tryParseJson(item));
      case metadataRecordInfo.Tag.Zmojis:
        return new MetadataRecord(tag, userMetadata.Zmojis.tryParseJson(item));
      default: throw staticFail(tag);
    }
  }

  toString() {
    return `MetadataRecord(${this.tag}, ${this.payload})`;
  }
}
