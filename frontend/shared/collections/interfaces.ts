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

export interface IKeyValuePair<K, V> {
  readonly key: K;
  value: V;
}

export interface IReadonlyMap<K, V> {
  containsKey(key: K): boolean;
  tryGet(key: K): V | undefined;
  lookupOrDefault<V2>(key: K, dflt: V2) : V | V2;

  tryGetExtremeKey(greatest: boolean) : K | undefined;
  tryGetExtremeEntry(greatest: boolean) : IKeyValuePair<K, V> | undefined;

  keys(forward?: boolean, startKey?: K, inclusive?: boolean) : IterableIterator<K>;
  values(forward?: boolean, startkey?: K, inclusive?: boolean) : IterableIterator<V>;
  entries(forward?: boolean, startKey?: K, inclusive?: boolean) : IterableIterator<IKeyValuePair<K, V>>;

  readonly size: number;
  readonly empty: boolean;
}

export interface IMap<K, V> extends IReadonlyMap<K, V> {
  add(key: K, value: V) : boolean;
  tryRemove(key: K): V | undefined;
}
