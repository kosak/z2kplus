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
  assertString,
}
  from "../../json_util";
import {staticFail} from "../../utility";
import {DRequest} from "../message/drequest";
import {Profile} from "../profile";

export namespace crequests {
  // Message from Oauth2-authenticating clients like the IOS client to identify themselves.
  export class Auth {
    constructor(public readonly token: string) {
    }

    toJson() {
      return [this.token];
    }

    static tryParseJson(item: any) {
      const [token] = assertAndDestructure1(item, assertString);
      return new Auth(token);
    }

    toString() {
      return `crequests.Auth(${this.token})`;
    }
  }

  // Identify yourself at the time of a new connection
  export class Hello {
    constructor(public readonly profile: Profile) {
    }

    toJson() {
      return [this.profile.toJson()];
    }

    static tryParseJson(item: any) {
      const [p] = assertAndDestructure1(item, Profile.tryParseJson);
      return new Hello(p);
    }

    toString() {
      return `crequests.Hello(${this.profile})`;
    }
  }

  // Create a new session with the server.
  export class CreateSession {
    constructor() {
    }

    toJson() {
      return [];
    }

    static tryParseJson(item: any) {
      assertAndDestructure0(item);
      return new CreateSession();
    }

    toString() {
      return `crequests.CreateSession()`;
    }
  }

  // Attach to an existing session
  export class AttachToSession {
    constructor(private readonly existingSessionId: string, private readonly nextExpectedResponseId: number) {
    }

    toJson() {
      return [this.existingSessionId, this.nextExpectedResponseId];
    }

    static tryParseJson(item: any) {
      let [e, n] = assertAndDestructure2(item, assertString, assertNumber);
      return new AttachToSession(e, n);
    }

    toString() {
      return `crequests.AttachToSession(${this.existingSessionId}, ${this.nextExpectedResponseId})`;
    }
  }

  export class PackagedRequest {
    constructor(readonly requestId: number, readonly nextExpectedResponseId: number, readonly request: DRequest) {
    }

    toJson() {
      return [this.requestId, this.nextExpectedResponseId, this.request.toJson()];
    }

    static tryParseJson(item: any) {
      let [r, n, q] = assertAndDestructure3(item, assertNumber, assertNumber, DRequest.tryParseJson);
      return new PackagedRequest(r, n, q);
    }

    toString() {
      return `crequests.PackagedRequest(${this.requestId}, ${this.nextExpectedResponseId}, ${this.request.toString()})`;
    }
  }
}

export namespace crequestInfo {
  export const enum Tag {
    Auth = "Auth",
    Hello = "Hello",
    CreateSession = "CreateSession",
    AttachToSession = "AttachToSession",
    PackagedRequest = "PackagedRequest"
  }

  export type payload_t = crequests.Auth | crequests.Hello | crequests.CreateSession | crequests.AttachToSession |
      crequests.PackagedRequest;

// This drama is to support the discriminated union pattern.
  interface IAuth {
    readonly tag: Tag.Auth;
    readonly payload: crequests.Auth;
  }
  interface IHello {
    readonly tag: Tag.Hello;
    readonly payload: crequests.Hello;
  }
  interface ICreateSession {
    readonly tag: Tag.CreateSession;
    readonly payload: crequests.CreateSession;
  }
  interface IAttachToSession {
    readonly tag: Tag.AttachToSession;
    readonly payload: crequests.AttachToSession;
  }
  interface IPackagedRequest {
    readonly tag: Tag.PackagedRequest;
    readonly payload: crequests.PackagedRequest;
  }

  export type ICRequest = IAuth | IHello | ICreateSession | IAttachToSession | IPackagedRequest;

  export interface IVisitor {
    visitAuth(payload: crequests.Auth) : void;
    visitHello(payload: crequests.Hello) : void;
    visitCreateSession(payload: crequests.CreateSession) : void;
    visitAttachToSession(payload: crequests.AttachToSession) : void;
    visitPackagedRequest(payload: crequests.PackagedRequest) : void;
  }
}

export class CRequest {
  static createAuth(token: string) {
    const payload = new crequests.Auth(token);
    return new CRequest(crequestInfo.Tag.Auth, payload);
  }

  static createHello(profile: Profile) {
    const payload = new crequests.Hello(profile);
    return new CRequest(crequestInfo.Tag.Hello, payload);
  }

  static createCreateSession() {
    const payload = new crequests.CreateSession();
    return new CRequest(crequestInfo.Tag.CreateSession, payload);
  }

  static createAttachToSession(existingSessionId: string, nextExpectedResponseId: number) {
    const payload = new crequests.AttachToSession(existingSessionId, nextExpectedResponseId);
    return new CRequest(crequestInfo.Tag.AttachToSession, payload);
  }

  static createPackagedRequest(requestId: number, nextExpectedResponseId: number, request: DRequest) {
    const payload = new crequests.PackagedRequest(requestId, nextExpectedResponseId, request);
    return new CRequest(crequestInfo.Tag.PackagedRequest, payload);
  }

  static createPackagedRequestFromPayload(payload: crequests.PackagedRequest) {
    return new CRequest(crequestInfo.Tag.PackagedRequest, payload);
  }

  constructor(readonly tag : crequestInfo.Tag, readonly payload: crequestInfo.payload_t) {
  }

  acceptVisitor(visitor: crequestInfo.IVisitor) {
    const icreq = this as crequestInfo.ICRequest;
    switch (icreq.tag) {
      case crequestInfo.Tag.Auth: {
        visitor.visitAuth(icreq.payload);
        break;

      }
      case crequestInfo.Tag.Hello: {
        visitor.visitHello(icreq.payload);
        break;
      }

      case crequestInfo.Tag.CreateSession: {
        visitor.visitCreateSession(icreq.payload);
        break;
      }

      case crequestInfo.Tag.AttachToSession: {
        visitor.visitAttachToSession(icreq.payload);
        break;
      }

      case crequestInfo.Tag.PackagedRequest: {
        visitor.visitPackagedRequest(icreq.payload);
        break;
      }

      default: staticFail(icreq);
    }
  }

  toJson() {
    const variant = [this.tag, this.payload.toJson()];
    // Serialized as if we had one member, namely payload, which is the variant.
    return [variant];
  }

  toString() {
    return `CRequest(${this.tag}, ${this.payload})`;
  }

  static tryParseJson(item: any) {
    const variant = assertAndDestructure1(item, i => assertArrayOfLength(i, 2));
    const [tag, item1] = assertAndDestructure2(variant, i => assertEnum<crequestInfo.Tag>(i), i => i);
    let payload: crequestInfo.payload_t;
    switch (tag) {
      case crequestInfo.Tag.Auth: {
        payload = crequests.Auth.tryParseJson(item1);
        break;
      }

      case crequestInfo.Tag.Hello: {
        payload = crequests.Hello.tryParseJson(item1);
        break;
      }

      case crequestInfo.Tag.CreateSession: {
        payload = crequests.CreateSession.tryParseJson(item1);
        break;
      }

      case crequestInfo.Tag.AttachToSession: {
        payload = crequests.AttachToSession.tryParseJson(item1);
        break;
      }

      case crequestInfo.Tag.PackagedRequest: {
        payload = crequests.PackagedRequest.tryParseJson(item1);
        break;
      }

      default: throw staticFail(tag);
    }
    return new CRequest(tag, payload);
  }
}
