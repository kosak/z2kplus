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

export class RateLimiter {
  private lastTransmit: number;
  private timer: number | undefined;
  private pending: Map<object, () => void>;

  constructor(private readonly frequencyMs: number) {
    this.lastTransmit = 0;
    this.timer = undefined;
    this.pending = new Map();
  }

  invoke(key: object, callback: () => void) {
    const keyAlreadyPresent = this.pending.get(key) !== undefined;
    this.pending.set(key, callback);
    if (keyAlreadyPresent) {
      return;
    }

    if (this.timer !== undefined) {
      // Already have a timer waiting to fire.
      return;
    }

    const now = Date.now();
    const elapsed = now - this.lastTransmit;
    const remaining = this.frequencyMs - elapsed;
    if (remaining <= 0) {
      // No timer, but it's been a while since the last one. Run immediately
      this.fireEventsAndReset();
      return;
    }

    // Defer this work for 'remaining' milliseconds
    this.timer = window.setTimeout(() => {
      this.fireEventsAndReset();
    }, remaining);
  }

  private fireEventsAndReset() {
    const temp = this.pending;
    this.lastTransmit = Date.now();
    this.timer = undefined;
    this.pending = new Map();
    for (let entry of temp.values()) {
      entry();
    }
  }
}
