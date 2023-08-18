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
  assertAndDestructure0,
  assertAndDestructure1,
  assertAndDestructure2,
  assertAndDestructure3, assertArrayOfLength,
  assertEnum,
  assertNumber,
  assertString
} from "../../json_util";
import {staticFail} from "../../utility";
import {DResponse} from "../message/dresponse";
import {Profile} from "../profile";

export namespace cresponses {
  export class SessionSuccess {
    constructor(readonly assignedSessionId: string, readonly nextExpectedRequestId: number,
                readonly profile: Profile) {}

    toJson() {
      return [this.assignedSessionId, this.nextExpectedRequestId, this.profile.toJson()];
    }

    static tryParseJson(item: any) {
      const [s, n, p] = assertAndDestructure3(item, assertString, assertNumber, Profile.tryParseJson);
      return new SessionSuccess(s, n, p);
    }

    toString() {
      return `cresponses.SessionSuccess(${this.assignedSessionId}, ${this.nextExpectedRequestId}, ${this.profile})`;
    }
  }

  export class SessionFailure {
    constructor() {}

    toJson() {
      return [];
    }

    static tryParseJson(item: any) {
      assertAndDestructure0(item);
      return new SessionFailure();
    }

    toString() {
      return `cresponses.SessionFailure()`;
    }
  }

  export class PackagedResponse {
    constructor(readonly responseId: number, readonly nextExpectedRequestId: number,
                readonly response: DResponse) {}

    toJson() {
      return [this.responseId, this.nextExpectedRequestId, this.response.toJson()];
    }

    static tryParseJson(item: any) {
      const [r, n, resp]  = assertAndDestructure3(item, assertNumber, assertNumber,
          DResponse.tryParseJson);
      return new PackagedResponse(r, n, resp);
    }

    toString() {
      return `cresponses.PackagedResponse(${this.responseId}, ${this.nextExpectedRequestId}, ${this.response.toString()}`;
    }
  }
}

export namespace cresponseInfo {
  export const enum Tag {
    SessionSuccess = "SessionSuccess",
    SessionFailure = "SessionFailure",
    PackagedResponse = "PackagedResponse"
  }

  export type payload_t = cresponses.SessionSuccess | cresponses.SessionFailure | cresponses.PackagedResponse;

  interface ISessionSuccess {
    readonly tag: Tag.SessionSuccess;
    readonly payload: cresponses.SessionSuccess;
  }
  interface ISessionFailure {
    readonly tag: Tag.SessionFailure;
    readonly payload: cresponses.SessionFailure;
  }
  interface IPackagedResponse {
    readonly tag: Tag.PackagedResponse;
    readonly payload: cresponses.PackagedResponse;
  }
  export type ICResponse = ISessionSuccess | ISessionFailure | IPackagedResponse;

  export interface IVisitor {
    visitSessionSuccess(payload: cresponses.SessionSuccess) : void;
    visitSessionFailure(payload: cresponses.SessionFailure) : void;
    visitPackagedResponse(payload: cresponses.PackagedResponse) : void;
  }
}

export class CResponse {
  static createSessionSuccess(assignedSessionId: string, nextExpectedRequestId: number, profile: Profile) {
    const payload = new cresponses.SessionSuccess(assignedSessionId, nextExpectedRequestId, profile);
    return new CResponse(cresponseInfo.Tag.SessionSuccess, payload);
  }

  static createSessionFailure() {
    const payload = new cresponses.SessionFailure();
    return new CResponse(cresponseInfo.Tag.SessionFailure, payload);
  }

  static createPackagedResponse(responseId: number, nextExpectedRequestId: number, response: DResponse) {
    const payload = new cresponses.PackagedResponse(responseId, nextExpectedRequestId, response);
    return new CResponse(cresponseInfo.Tag.PackagedResponse, payload);
  }

  constructor(readonly tag: cresponseInfo.Tag, readonly payload: cresponseInfo.payload_t) {
  }

  acceptVisitor(visitor: cresponseInfo.IVisitor) {
    const icresp = this as cresponseInfo.ICResponse;
    switch (icresp.tag) {
      case cresponseInfo.Tag.SessionSuccess:
        visitor.visitSessionSuccess(icresp.payload);
        break;

      case cresponseInfo.Tag.SessionFailure:
        visitor.visitSessionFailure(icresp.payload);
        break;

      case cresponseInfo.Tag.PackagedResponse:
        visitor.visitPackagedResponse(icresp.payload);
        break;

      default:
        staticFail(icresp);
    }
  }

  toJson() {
    const variant = [this.tag, this.payload.toJson()];
    // Serialized as if we had one member, namely payload, which is the variant.
    return [variant];

  }

  static tryParseJson(item: any) {
    const [variant] = assertAndDestructure1(item, i => assertArrayOfLength(i, 2));
    const [tag, item1] = assertAndDestructure2(variant, i => assertEnum<cresponseInfo.Tag>(i), i => i);
    let payload;
    switch (tag) {
      case cresponseInfo.Tag.SessionSuccess:
        payload = cresponses.SessionSuccess.tryParseJson(item1);
        break;
      case cresponseInfo.Tag.SessionFailure:
        payload = cresponses.SessionFailure.tryParseJson(item1);
        break;
      case cresponseInfo.Tag.PackagedResponse:
        payload = cresponses.PackagedResponse.tryParseJson(item1);
        break;
      default:
        throw staticFail(tag);
    }
    return new CResponse(tag, payload);
  }

  toString() {
    return `CResponse(${this.tag}, ${this.payload})`;
  }
}
