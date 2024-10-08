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

export namespace magicConstants {
  export const initialQuerySize = 20;
  export const pageSize = 50;
  export const queryMargin = 20;

  // The max ID is 2^26 - 1  (approximately 64 million). Increase this number to support more ids
  export const numBitsInId : number = 26;
  export const uint32Max = Math.pow(2, 32) - 1;
  export const referredSummaryLimit = 50;
  export const parseCheckIntervalMsec = 500;
  export const frontendReconnectIntervalMs = 5 * 1000;
  export const backendReconnectIntervalMs = 10 * 1000;

  export const maxPingResponseTimeMs = 10 * 1000;
  export const pingIntervalMs = 30 * 1000;

  export const z2kVirtualMediaPrefix = "z2kplusmedia://";

  export const filtersLocalStorageKey = "filters";
}
