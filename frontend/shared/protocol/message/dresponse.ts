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

import {Estimates, Filter} from "../misc";
import {MetadataRecord, Zephyrgram, ZgramId} from "../zephyrgram";
import {
  assertAndDestructure1, assertAndDestructure2, assertAndDestructure3,
  assertArray, assertArrayOfLength, assertBoolean, assertEnum, assertNumber, assertString
} from "../../json_util";
import {intercalate, staticFail} from "../../utility";

export namespace dresponses {
  export class AckSyntaxCheck {
    constructor(readonly query: string, readonly valid: boolean, readonly result: string) {}

    toJson() {
      return [this.query, this.valid, this.result];
    }

    static tryParseJson(item: any) {
      let [q, v, r] = assertAndDestructure3(item, assertString, assertBoolean, assertString);
      return new AckSyntaxCheck(q, v, r);
    }

    toString() {
      return `AckSyntaxCheck(${this.query}, ${this.valid}, ${this.result})`;
    }
  }

  export class AckSubscribe {
    constructor(readonly valid: boolean, readonly humanReadableError: string, readonly estimates: Estimates) {
    }

    toJson() {
      return [this.valid, this.humanReadableError, this.estimates.toJson()];
    }

    static tryParseJson(item: any) {
      let [v, h, e] = assertAndDestructure3(item, assertBoolean, assertString, Estimates.tryParseJson);
      return new AckSubscribe(v, h, e);
    }
  }

  export class EstimatesUpdate {
    constructor(readonly estimates: Estimates) {
    }

    toJson() {
       return [this.estimates.toJson()];
    }

    static tryParseJson(item: any) {
      const [es] = assertAndDestructure1(item, Estimates.tryParseJson);
      return new EstimatesUpdate(es);
    }

    toString() {
      return `Estimates(${this.estimates})`;
    }
  }

  export class AckMoreZgrams {
    constructor(readonly forBackSide: boolean, readonly zgrams: Zephyrgram[], readonly estimates: Estimates) {
    }

    toJson() {
      const zgJson = this.zgrams.map(i => i.toJson());
      return [this.forBackSide, zgJson, this.estimates.toJson()];
    }

    static tryParseJson(item: any) {
      const [bs, zgItems, es] = assertAndDestructure3(item, assertBoolean, assertArray,
          Estimates.tryParseJson);
      const zgs = zgItems.map(Zephyrgram.tryParseJson);
      return new AckMoreZgrams(bs, zgs, es);
    }

    toString() {
      return `MoreZgrams(${this.forBackSide}, ${this.zgrams}, ${this.estimates})`;
    }
  }

  export class MetadataUpdate {
    constructor(readonly metadata: MetadataRecord[]) {
    }

    toJson() {
      const mdJson = this.metadata.map(i => i.toJson());
      return [mdJson];
    }

    static tryParseJson(item: any) {
      const [mdItems] = assertAndDestructure1(item, assertArray);
      const mds = mdItems.map(MetadataRecord.tryParseJson);
      return new MetadataUpdate(mds);
    }

    toString() {
      return `MetadataResponse(${this.metadata})`;
    }
  }

  export class AckSpecificZgrams {
    constructor(readonly zgrams: Zephyrgram[]) {
    }

    toJson() {
      const zgJson = this.zgrams.map(i => i.toJson());
      return [zgJson];
    }

    static tryParseJson(item: any) {
      const [zgItems] = assertAndDestructure1(item, assertArray);
      const zgs = zgItems.map(Zephyrgram.tryParseJson);
      return new AckSpecificZgrams(zgs);
    }

    toString() {
      return `AckSpecificZgrams(${this.zgrams})`;
    }
  }

  export class PlusPlusUpdate {
    constructor(readonly entries: [ZgramId, string, number][]) {
    }

    toJson() {
      const mapped = this.entries.map(e => {
        return [e[0].toJson(), e[1], e[2]];
      });
      return [mapped];
    }

    static tryParseJson(item: any) {
      const [entries] = assertAndDestructure1(item, assertArray);
      const converted = entries.map(e => {
        return assertAndDestructure3(e, ZgramId.tryParseJson, assertString, assertNumber);
      });
      return new PlusPlusUpdate(converted);
    }

    toString() {
      return `PlusPlusUpdate(${this.entries})`;
    }
  }

  export class AckPing {
    constructor(readonly cookie: number) {
    }

    toJson() {
      return [this.cookie];
    }

    static tryParseJson(item: any) {
      const [cookie] = assertAndDestructure1(item, assertNumber);
      return new AckPing(cookie);
    }

    toString() {
      return `Ack(${this.cookie})`;
    }
  }

  export class FiltersUpdate {
    constructor(readonly version: number, readonly filters: Filter[]) {
    }

    toJson() {
      return [this.version, this.filters.map(f => f.toJson())];
    }

    static tryParseJson(item: any) {
      const [v, fs] = assertAndDestructure2(item, assertNumber, assertArray);
      const filters = fs.map(Filter.tryParseJson);
      return new FiltersUpdate(v, filters);
    }

    toString() {
      var text = intercalate(", ", x => x.toString(), this.filters);
      return `FiltersUpdate(${this.version}, ${text})`;
    }
  }

  export class GeneralError {
    constructor(readonly message: string) {
    }

    toJson() {
      return [this.message];
    }

    static tryParseJson(item: any) {
      const [m] = assertAndDestructure1(item, assertString);
      return new GeneralError(m);
    }

    toString() {
      return `GeneralError(${this.message})`;
    }
  }
}

export namespace dresponseInfo {
  export const enum Tag {
    // The response to Request.SyntaxCheck
    AckSyntaxCheck = "AckSyntaxCheck",
    // The response to Request.SyntaxCheck
    AckSubscribe = "AckSubscribe",
    // The response to GetMoreZgrams
    AckMoreZgrams = "AckMoreZgrams",
    // Estimates
    EstimatesUpdate = "EstimatesUpdate",
    // Metadata.
    MetadataUpdate = "MetadataUpdate",
    // The response to Request.GetSpecificZgrams
    AckSpecificZgrams = "AckSpecificZgrams",
    // Plusplus values
    PlusPlusUpdate = "PlusPlusUpdate",
    // Setting filters
    FiltersUpdate = "FiltersUpdate",
    // For debugging. An acknowledgement of a Ping.
    AckPing = "AckPing",
    // When the server wants to complain about something but has nowhere else to say it.
    GeneralError = "GeneralError"
  }

  export type payload_t =
      dresponses.AckSyntaxCheck | dresponses.AckSubscribe | dresponses.EstimatesUpdate |
      dresponses.AckMoreZgrams | dresponses.MetadataUpdate | dresponses.AckSpecificZgrams |
      dresponses.PlusPlusUpdate | dresponses.FiltersUpdate | dresponses.AckPing | dresponses.GeneralError;

  export interface IAckSyntaxCheck {
    readonly tag: Tag.AckSyntaxCheck;
    readonly payload: dresponses.AckSyntaxCheck;
  }

  export interface IAckSubscribe {
    readonly tag: Tag.AckSubscribe;
    readonly payload: dresponses.AckSubscribe;
  }

  export interface IAckMoreZgrams {
    readonly tag: Tag.AckMoreZgrams;
    readonly payload: dresponses.AckMoreZgrams;
  }

  export interface IEstimatesUpdate {
    readonly tag: Tag.EstimatesUpdate;
    readonly payload: dresponses.EstimatesUpdate;
  }

  export interface IMetadataUpdate {
    readonly tag: Tag.MetadataUpdate;
    readonly payload: dresponses.MetadataUpdate;
  }

  export interface IAckSpecificZgrams {
    readonly tag: Tag.AckSpecificZgrams;
    readonly payload: dresponses.AckSpecificZgrams;
  }

  export interface IPlusPlusUpdate {
    readonly tag: Tag.PlusPlusUpdate;
    readonly payload: dresponses.PlusPlusUpdate;
  }

  export interface IFiltersUpdate {
    readonly tag: Tag.FiltersUpdate;
    readonly payload: dresponses.FiltersUpdate;
  }

  export interface IAckPing {
    readonly tag: Tag.AckPing;
    readonly payload: dresponses.AckPing;
  }

  export interface IGeneralError {
    readonly tag: Tag.GeneralError;
    readonly payload: dresponses.GeneralError;
  }

  export interface IVisitor<TResult> {
    visitAckSyntaxCheck(payload: dresponses.AckSyntaxCheck): TResult;

    visitAckSubscribe(payload: dresponses.AckSubscribe): TResult;

    visitAckMoreZgrams(payload: dresponses.AckMoreZgrams): TResult;

    visitAckSpecificZgrams(payload: dresponses.AckSpecificZgrams): TResult;

    visitEstimatesUpdate(payload: dresponses.EstimatesUpdate): TResult;

    visitMetadataUpdate(payload: dresponses.MetadataUpdate): TResult;

    visitPlusPlusUpdate(payload: dresponses.PlusPlusUpdate): TResult;

    visitFiltersUpdate(payload: dresponses.FiltersUpdate): TResult;

    visitAckPing(payload: dresponses.AckPing): TResult;

    visitGeneralError(o: dresponses.GeneralError): TResult;
  }
}  // namespace dresponseInfo

export type IDResponse =
    dresponseInfo.IAckSyntaxCheck | dresponseInfo.IAckSubscribe |
    dresponseInfo.IEstimatesUpdate | dresponseInfo.IAckMoreZgrams |
    dresponseInfo.IAckSpecificZgrams |
    dresponseInfo.IMetadataUpdate | dresponseInfo.IPlusPlusUpdate |
    dresponseInfo.IFiltersUpdate | dresponseInfo.IAckPing |
    dresponseInfo.IGeneralError;

export class DResponse {
  static createAckSyntaxCheck(query: string, valid: boolean, result: string) {
    return new DResponse(dresponseInfo.Tag.AckSyntaxCheck,
        new dresponses.AckSyntaxCheck(query, valid, result));
  }
  static createAckSubscribe(valid: boolean, humanReadableError: string, estimates: Estimates) {
    return new DResponse(dresponseInfo.Tag.AckSubscribe,
        new dresponses.AckSubscribe(valid, humanReadableError, estimates));
  }
  static createEstimatesUpdate(estimates: Estimates) {
    return new DResponse(dresponseInfo.Tag.EstimatesUpdate,
        new dresponses.EstimatesUpdate(estimates));
  }
  static createPlusPlusUpdate(entries: [ZgramId, string, number][]) {
    return new DResponse(dresponseInfo.Tag.PlusPlusUpdate,
        new dresponses.PlusPlusUpdate(entries));
  }
  static createFiltersUpdate(version: number, filters: Filter[]) {
    return new DResponse(dresponseInfo.Tag.FiltersUpdate,
        new dresponses.FiltersUpdate(version, filters));
  }
  static createAckMoreZgrams(forBackSide: boolean, zgrams: Zephyrgram[], estimates: Estimates) {
    return new DResponse(dresponseInfo.Tag.AckMoreZgrams,
        new dresponses.AckMoreZgrams(forBackSide, zgrams, estimates));
  }
  static createAckPing(cookie: number) {
    return new DResponse(dresponseInfo.Tag.AckPing, new dresponses.AckPing(cookie));
  }
  static createGeneralError(message: string) {
    return new DResponse(dresponseInfo.Tag.GeneralError, new dresponses.GeneralError(message));
  }

  constructor(readonly tag: dresponseInfo.Tag, readonly payload: dresponseInfo.payload_t) {
  }

  acceptVisitor<TResult>(visitor: dresponseInfo.IVisitor<TResult>) : TResult {
    const idresp = this as IDResponse;
    switch (idresp.tag) {
      case dresponseInfo.Tag.AckSyntaxCheck:
        return visitor.visitAckSyntaxCheck(idresp.payload);
      case dresponseInfo.Tag.AckSubscribe:
        return visitor.visitAckSubscribe(idresp.payload);
      case dresponseInfo.Tag.EstimatesUpdate:
        return visitor.visitEstimatesUpdate(idresp.payload);
      case dresponseInfo.Tag.AckMoreZgrams:
        return visitor.visitAckMoreZgrams(idresp.payload);
      case dresponseInfo.Tag.MetadataUpdate:
        return visitor.visitMetadataUpdate(idresp.payload);
      case dresponseInfo.Tag.AckSpecificZgrams:
        return visitor.visitAckSpecificZgrams(idresp.payload);
      case dresponseInfo.Tag.PlusPlusUpdate:
        return visitor.visitPlusPlusUpdate(idresp.payload);
      case dresponseInfo.Tag.FiltersUpdate:
        return visitor.visitFiltersUpdate(idresp.payload);
      case dresponseInfo.Tag.AckPing:
        return visitor.visitAckPing(idresp.payload);
      case dresponseInfo.Tag.GeneralError:
        return visitor.visitGeneralError(idresp.payload);
      default:
        staticFail(idresp);
    }
  }

  toJson() {
    const variant = [this.tag, this.payload.toJson()];
    return [variant];
  }

  static tryParseJson(item: any) {
    const [variant] = assertAndDestructure1(item, i => assertArrayOfLength(i, 2));
    const [tag, item1] = assertAndDestructure2(variant, i => assertEnum<dresponseInfo.Tag>(i), i => i);
    let payload: dresponseInfo.payload_t;
    switch (tag) {
      case dresponseInfo.Tag.AckSyntaxCheck: payload = dresponses.AckSyntaxCheck.tryParseJson(item1); break;
      case dresponseInfo.Tag.AckSubscribe: payload = dresponses.AckSubscribe.tryParseJson(item1); break;
      case dresponseInfo.Tag.EstimatesUpdate: payload = dresponses.EstimatesUpdate.tryParseJson(item1); break;
      case dresponseInfo.Tag.AckMoreZgrams: payload = dresponses.AckMoreZgrams.tryParseJson(item1); break;
      case dresponseInfo.Tag.MetadataUpdate: payload = dresponses.MetadataUpdate.tryParseJson(item1); break;
      case dresponseInfo.Tag.AckSpecificZgrams: payload = dresponses.AckSpecificZgrams.tryParseJson(item1); break;
      case dresponseInfo.Tag.PlusPlusUpdate: payload = dresponses.PlusPlusUpdate.tryParseJson(item1); break;
      case dresponseInfo.Tag.FiltersUpdate: payload = dresponses.FiltersUpdate.tryParseJson(item1); break;
      case dresponseInfo.Tag.AckPing: payload = dresponses.AckPing.tryParseJson(item1); break;
      case dresponseInfo.Tag.GeneralError: payload = dresponses.GeneralError.tryParseJson(item1); break;
      default: throw staticFail(tag);
    }
    return new DResponse(tag, payload);
  }

  toString() {
    return `DResponse(${this.tag}, ${this.payload})`;
  }
}
