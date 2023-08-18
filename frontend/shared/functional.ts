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

export function *concat<T>(lhs: Iterable<T>, rhs: Iterable<T>) : IterableIterator<T> {
  {
    let lIter = lhs[Symbol.iterator]();
    while (true) {
      let next = lIter.next();
      if (next.done) {
        break;
      }
      yield next.value;
    }
  }
  let rIter = rhs[Symbol.iterator]();
  while (true) {
    let next = rIter.next();
    if (next.done) {
      break;
    }
    yield next.value;
  }
}

export function *skip<T>(items: Iterable<T>, count: number) : IterableIterator<T> {
  let iter = items[Symbol.iterator]();
  while (count > 0) {
    let next = iter.next();
    if (next.done) {
      return;
    }
    --count;
  }
  while (true) {
    let next = iter.next();
    if (next.done) {
      return;
    }
    yield next.value;
  }
}

export function *zip<T1,T2>(lhs: Iterable<T1>, rhs: Iterable<T2>) : IterableIterator<[T1,T2]> {
  let lIter = lhs[Symbol.iterator]();
  let rIter = rhs[Symbol.iterator]();
  while (true) {
    let lNext = lIter.next();
    let rNext = rIter.next();

    let lDone = lNext.done;
    let rDone = rNext.done;

    if (lDone !== rDone) {
      throw new Error(`Iterables don't have same length: ${lDone}, ${rDone}`);
    }

    if (lDone) {
      return;
    }

    yield [lNext.value, rNext.value];
  }
}
