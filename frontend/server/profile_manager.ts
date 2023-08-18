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

import {Profile} from "../shared/protocol/profile";
import * as fs from "fs";
import {readKeyValueFile} from "./util";

export class ProfileManager {
  private userIdToProfile: {[key: string]: Profile};

  constructor(private readonly z2kProfilesFile: string) {
    this.userIdToProfile = {};
    this.populate();
    fs.watch(z2kProfilesFile, (_1, _2) => {
      this.populate();
    });
  }

  populate() {
    const result: {[key: string]: Profile} = {};
    const pairs = readKeyValueFile(this.z2kProfilesFile);
    if (pairs === undefined) {
      return;
    }
    for (const [userId, signature] of pairs) {
      if (result[userId] !== undefined) {
        console.log(`Duplicate userId: ${userId}`);
        return;
      }
      result[userId] = new Profile(userId, signature);
    }
    this.userIdToProfile = result;
  }

  public tryGetProfile(userId: string) {
    return this.userIdToProfile[userId];
  }
}
