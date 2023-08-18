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

import {magicConstants} from "./magic_constants";

export class Chunker {
  private pending: string;

  constructor() {
    this.pending = "";
  }

  static wrap(request: string) {
    return request + '\n';
  }

  pushBack(text: string) {
    this.pending += text;
  }

  tryUnwrapNext() : string | undefined {
    const nextTerminator = this.pending.indexOf('\n');
    if (nextTerminator < 0) {
      return undefined;
    }
    const result = this.pending.substring(0, nextTerminator);
    this.pending = this.pending.substring(nextTerminator + 1);
    return result;
  }
}
