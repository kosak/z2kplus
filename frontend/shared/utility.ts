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

// This is used e.g. to detect missing case statments at compile time
import {anyToJson, assertAndDestructure1, assertAndDestructure2, assertArray} from "./json_util";

export function operatorToDelta(operator: string) {
  switch (operator) {
    case "++": return 1;
    case "--": return -1;
    case "??":
    case "~~": return 0;
    default: throw `I hate you ${operator}`;
  }
}

export function tryParseWholeNumber(s: string) {
  if (!s.match(/^[0-9]+$/)) {
    return undefined;
  }
  return parseInt(s);
}

export function exceptionAsString(e: any) {
  if (e instanceof Error) {
    return e.message;
  }
  return e.toString();
}

export function staticFail(arg: never) : never {
  throw new Error("notreached: " + arg);
}

export function escape(s: string) {
  return s.replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/'/g, "&apos;")
    .replace(/"/g, "&quot;");
}

export function escapeQuotes(s: string) {
  return s.replace(/"/g, '\\"');
}

export function linkify(s: string) {
  return s.replace(/https?:\/\/\S+/g, '<a href="$&" target="_blank">$&</a>');
}

export function translateNewline(s: string) {
  return s.replace(/\n/g, "<br>");
}

export function shuffle(array: any[]) {
  for (let size = array.length; size > 1; --size) {
    const i = size - 1;
    const j = Math.floor(Math.random() * size);
    if (i === j) {
      continue;
    }
    const temp = array[i];
    array[i] = array[j];
    array[j] = temp;
  }
}

export function isLoggableInstance(instance: string) {
  return instance !== "graffiti" &&
    instance.substring(0, 9) !== "graffiti.";
}

export function getUnixEpochTime() {
  return Math.floor(Date.now() / 1000);
}

export function toUnixTimesecs(d: Date) {
  return Math.floor(d.getTime() / 1000);
}

function getQcodingRegex() {
  // Don't reuse the same regex, because JavaScript keeps enumeration state inside the Regexp
  // object.
  return new RegExp("q(.q)+", "uig");
}

// A q-decodable region of a string is an run of characters where every other character is q or Q,
// starting with q or Q. There may be multiple such runs in the target string.
// Example: I like pie but not qcqaqkqeq and not qcqiqnqnqaqbqqoqnq.
// qDecoding deconstes such runs but tries to not damage the rest of the string. Note: needs to be
// unicode-aware (hence the u flag).
export function isQDecodable(s: string) {
  return getQcodingRegex().test(s);
}

export function qDecode(s: string) {
  // Each match ends with a q so trim that away (capture.length - 1)
  return s.replace(getQcodingRegex(), match => {
    const matchAfterInitialQ = match.substring(1);
    return matchAfterInitialQ.replace(/.q/iug, pair => pair.substring(0, pair.length - 1));
  });
}

// qqq-qeqnqcqoqdqeq qtqhqeq qmqeqsqsqaqgqeq :-)
export function qEncode(s: string) : string {
  if (s.length === 0) {
    return "";
  }
  const result = s.replace(/./ug, (match: string) => match + (isUpper(match) ? "Q" : "q"));
  const first = isUpper(result.charAt(0)) ? "Q" : "q";
  return first + result;
}

export function isUpper(ch: string) {
  return ch.toUpperCase() == ch && ch.toLowerCase() != ch;
}

export interface IComparable<T> {
  compareTo(other: T) : number;
}

export function booleanCompare(lhs: boolean, rhs: boolean) {
  if (lhs === rhs) {
    return 0;
  }
  return !lhs ? -1 : 1;
}

export function stringCompare(lhs: string, rhs: string) {
  if (lhs === rhs) {
    return 0;
  }
  return lhs < rhs ? -1 : 1;
}

export class Pair<First, Second> {
  static create<First, Second>(first: First, second: Second) {
    return new Pair<First, Second>(first, second);
  }

  constructor(public first: First, public second: Second) {
  }

  static tryParseJson<First, Second>(item: any, fj: (i: any) => First, sj: (i: any) => Second) {
    const [first, second] = assertAndDestructure2(item, fj, sj);
    return new Pair<First, Second>(first, second);
  }

  toJson(firstToJson?: (f: First) => any, secondToJson?: (s: Second) => any) {
    if (firstToJson === undefined) {
      firstToJson = anyToJson;
    }
    if (secondToJson === undefined) {
      secondToJson = anyToJson;
    }
    return [firstToJson(this.first), secondToJson(this.second)];
  }

  toString() {
    return `[${this.first}, ${this.second}]`;
  }
}

// Does this work? Do I want it?
export class Optional<T> {
  static empty<T>() {
    return new Optional<T>(undefined);
  }

  static create<T>(item: T | undefined) {
    return new Optional<T>(item);
  }

  constructor(public readonly item?: T) {
  }

  static tryParseJson<T>(item: any, ij: (i: any) => T) : Optional<T> {
    const [array] = assertAndDestructure1(item, assertArray);
    if (array.length === 0) {
      return Optional.empty();
    }
    const element = ij(array[0]);
    return Optional.create(element);
  }

  toJson() {
    if (this.item === undefined) {
      return [];
    }
    return [anyToJson(this.item)];
  }

  toString() {
    return `[${this.item}]`;
  }
}

export class Interval<T> {
  static create<T>(begin: T, end: T) {
    return new Interval<T>(begin, end);
  }

  constructor(begin: T, end: T) {
    this._begin = begin;
    this._end = end;
  }

  toString() {
    return `[${this._begin}, ${this._end})`;
  }

  _begin: T;
  _end: T;
}

export function makeCommaSeparatedList(items: any[]) {
  let comma = "";
  let result = "";
  for (let item of items) {
    result += comma;
    result += item.toString();
    comma = ", ";
  }
  return result;
}

export function intercalate<T>(separator: string, toString: (item: T) => string, ...items: T[]) {
  let separatorToUse = "";
  let result = "";
  for (let item of items) {
    result += separatorToUse;
    result += toString(item);
    separatorToUse = separator;
  }
  return result;
}
