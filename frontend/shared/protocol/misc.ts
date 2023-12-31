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
  assertAndDestructure2, assertAndDestructure6, assertArrayOfLength,
  assertBoolean, assertEnum, assertNumber, assertString, optionalToJson, optionalTryParseJson,
} from "../json_util";
import {booleanCompare, IComparable} from "../utility";

export class Unit implements IComparable<Unit> {
  private static readonly instance = new Unit();

  static create() {
    return Unit.instance;
  }

  private constructor() {}

  toJson() {
    return [];
  }

  compareTo(other: Unit): number {
    return 0;
  }

  static tryParseJson(item: any) {
    assertArrayOfLength(item, 0);
    return Unit.create();
  }

  toString() {
    return `Unit()`;
  }
}

export class Estimate {
  constructor(readonly count: number, readonly exact: boolean) {
  }

  toJson() {
    return [this.count, this.exact];
  }

  static tryParseJson(item: any) {
    let [c, lb] = assertAndDestructure2(item, assertNumber, assertBoolean);
    return new Estimate(c, lb);
  }

  compareTo(other: Estimate) {
    let difference = this.count - other.count;
    if (difference !== 0) return difference;
    difference = booleanCompare(this.exact, other.exact);
    return difference;
  }

  toString() {
    return `Estimate(${this.count}, ${this.exact})`;
  }
}

export class Estimates {
  constructor(readonly front: Estimate, readonly back: Estimate) {
  }

  toJson() {
    return [this.front.toJson(), this.back.toJson()];
  }

  static tryParseJson(item: any) {
    let [f, b] = assertAndDestructure2(item, Estimate.tryParseJson, Estimate.tryParseJson);
    return new Estimates(f, b);
  }

  compareTo(other: Estimates) {
    let difference: number;
    difference = this.front.compareTo(other.front);
    if (difference !== 0) return difference;
    difference = this.back.compareTo(other.back);
    return difference;
  }

  toString() {
    return `Estimate(${this.front}, ${this.back})`;
  }
}

export class Filter {
  constructor(
      readonly id: string,
      readonly sender: string | undefined, readonly instanceExact: string | undefined,
      readonly instancePrefix: string | undefined, readonly strong: boolean,
      readonly expirationSecs: number) {
  }

  toJson() {
    return [
        this.id,
        optionalToJson(this.sender, i => i),
        optionalToJson(this.instanceExact, i => i),
        optionalToJson(this.instancePrefix, i => i),
        this.strong,
        this.expirationSecs];
  }

  static tryParseJson(item: any) {
    let [id, sd, ix, ip, st, ex] = assertAndDestructure6(item,
        assertString,
        i => optionalTryParseJson(i, assertString),
        i => optionalTryParseJson(i, assertString),
        i => optionalTryParseJson(i, assertString),
        assertBoolean,
        assertNumber);
    return new Filter(id, sd, ix, ip, st, ex);
  }

  toString() {
    return `Filter(${this.id}, ${this.sender}, ${this.instanceExact}, ${this.instancePrefix}, ${this.strong}, ` +
      `${this.expirationSecs})`;
  }
}
