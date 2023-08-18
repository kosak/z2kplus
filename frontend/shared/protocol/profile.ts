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

import { assertAndDestructure2, assertString } from "../json_util";

export class Profile {
  static empty() : Profile {
    return new Profile("nobody", "Nobody");
  }

  constructor(readonly userId: string, readonly signature: string) {
  }

  toJson() {
    return [this.userId, this.signature];
  }

  static tryParseJson(item: any) {
    const [userId, signature] = assertAndDestructure2(item, assertString, assertString);
    return new Profile(userId, signature);
  }
}
