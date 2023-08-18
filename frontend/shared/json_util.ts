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

import {staticFail} from "./utility";

export interface IJsonable {
  toJson(): any;
}

export function assertBoolean(item: any) {
  if (typeof(item) !== "boolean") {
    throw new Error(`Expected a boolean, got ${Object.prototype.toString.call(item)}`);
  }
  return item as boolean;
}

export function assertNumber(item: any) {
  if (typeof(item) !== "number") {
    throw new Error(`Expected a number, got ${Object.prototype.toString.call(item)}`);
  }
  return item as number;
}

// This one is a little problematic.
export function assertEnum<T>(item: any) {
  if (typeof(item) !== "string") {
    throw new Error(
      `Expected a string (for string-valued enum), got ${Object.prototype.toString.call(item)}`);
  }
  return item as any as T;
}

export function assertString(item: any) {
  if (typeof(item) !== "string") {
    throw new Error(`Expected a string, got ${Object.prototype.toString.call(item)}`);
  }
  return item as string;
}

export function assertArray(item: any) {
  if (!Array.isArray(item)) {
    throw new Error(`Expected an array, got ${item}`);
  }
  return item as any[];
}

export function assertArrayOfLength(item: any, size: number) {
  let items = assertArray(item);
  if (items.length != size) {
    throw new Error(`Expected array of size ${size}, got size ${items.length}`);
  }
  return items;
}

export function anyToJson<T>(item: T) {
  if (typeof(item) === "boolean" || typeof(item) === "number" || typeof(item) === "string") {
    return item;
  }
  let ij = item as IJsonable;
  if (!ij.toJson) {
    throw new Error(`Object is not jsonable ${ij}`);
  }
  return ij.toJson();
}

export function optionalToJson<T>(item: T | undefined, toJson: (i: T) => any) {
  if (item === undefined) {
    return [];
  }
  return [toJson(item)];
}

export function optionalTryParseJson<T>(item: any, parseItem: (i: any) => T) : T | undefined {
  let arr = assertArray(item);
  if (arr.length == 0) {
    return undefined;
  }
  if (arr.length == 1) {
    return parseItem(arr[0]);
  }
  throw new Error(`Unexpected length for Optional: ${arr.length}`);
}

export function assertAndDestructure0(item: any) : [] {
  assertArrayOfLength(item, 0);
  return [];
}

export function assertAndDestructure1<V0>(item: any, fn0: (x: any) => V0) : [V0] {
  const items = assertArrayOfLength(item, 1);
  return [fn0(items[0])];
}

export function assertAndDestructure2<V0, V1>(item: any,
  fn0: (x: any) => V0,
  fn1: (x: any) => V1) : [V0, V1] {
  const items = assertArrayOfLength(item, 2);
  const r0 = fn0(items[0]);
  const r1 = fn1(items[1]);
  return [r0, r1];
}

export function assertAndDestructure3<V0, V1, V2>(item: any,
  fn0: (x: any) => V0,
  fn1: (x: any) => V1,
  fn2: (x: any) => V2) : [V0, V1, V2] {
  const items = assertArrayOfLength(item, 3);
  const r0 = fn0(items[0]);
  const r1 = fn1(items[1]);
  const r2 = fn2(items[2]);
  return [r0, r1, r2];
}

export function assertAndDestructure4<V0, V1, V2, V3>(item: any,
  fn0: (x: any) => V0,
  fn1: (x: any) => V1,
  fn2: (x: any) => V2,
  fn3: (x: any) => V3) : [V0, V1, V2, V3] {
  const items = assertArrayOfLength(item, 4);
  const r0 = fn0(items[0]);
  const r1 = fn1(items[1]);
  const r2 = fn2(items[2]);
  const r3 = fn3(items[3]);
  return [r0, r1, r2, r3];
}

export function assertAndDestructure5<V0, V1, V2, V3, V4>(item: any,
  fn0: (x: any) => V0,
  fn1: (x: any) => V1,
  fn2: (x: any) => V2,
  fn3: (x: any) => V3,
  fn4: (x: any) => V4) : [V0, V1, V2, V3, V4] {
  const items = assertArrayOfLength(item, 5);
  const r0 = fn0(items[0]);
  const r1 = fn1(items[1]);
  const r2 = fn2(items[2]);
  const r3 = fn3(items[3]);
  const r4 = fn4(items[4]);
  return [r0, r1, r2, r3, r4];
}

export function assertAndDestructure6<V0, V1, V2, V3, V4, V5>(item: any,
  fn0: (x: any) => V0,
  fn1: (x: any) => V1,
  fn2: (x: any) => V2,
  fn3: (x: any) => V3,
  fn4: (x: any) => V4,
  fn5: (x: any) => V5) : [V0, V1, V2, V3, V4, V5] {
  const items = assertArrayOfLength(item, 6);
  const r0 = fn0(items[0]);
  const r1 = fn1(items[1]);
  const r2 = fn2(items[2]);
  const r3 = fn3(items[3]);
  const r4 = fn4(items[4]);
  const r5 = fn5(items[5]);
  return [r0, r1, r2, r3, r4, r5];
}
