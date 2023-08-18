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

export class LinkedList<T> {
  private _head: LinkedListNode<T> | undefined;
  private _tail: LinkedListNode<T> | undefined;
  private _size: number;

  constructor() {
    this.clear();
  }

  clear() {
    this._head = undefined;
    this._tail = undefined;
    this._size = 0;
  }

  append(value: T) {
    ++this._size;
    const entry = new LinkedListNode(value);
    if (this._head === undefined) {
      this._head = entry;
      this._tail = entry;
      return;
    }
    this._tail!.next = entry;
    this._tail = entry;
  }

  *entries() {
    for (let item = this._head; item !== undefined; item = item.next) {
      yield item.value;
    }
  }

  empty() {
    return this._head == undefined;
  }

  front() {
    return this.safeHead.value;
  }

  tryRemoveFront() {
    const head = this._head;
    if (head === undefined) {
      return undefined;
    }
    if (head === this._tail) {
      this._head = undefined;
      this._tail = undefined;
    } else {
      this._head = head.next;
    }
    --this._size;
    return head.value;
  }

  get safeHead() {
    if (this._head === undefined) {
      throw new Error(`BAD`);
    }
    return this._head;
  }

  get size() {
    return this._size;
  }
}

export class LinkedListNode<T> {
  next: LinkedListNode<T> | undefined;

  constructor(public readonly value: T) {
    this.next = undefined;
  }

  toString() {
    return `LinkedListNode(${this.value})`;
  }
}
