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

import * as bcrypt from "bcrypt"
import * as fs from "fs"
import {readKeyValueFile} from "./util";

export class AuthorizationManager {
  private googleProfileIdToUserId: {[key: string]: string};
  private userIdToHash: {[key: string]: string};

  constructor(private readonly googlePlusFile: string, private readonly saltedPasswordFile: string) {
    this.googleProfileIdToUserId = {};
    this.userIdToHash = {};

    this.populateGooglePlusProfile();
    this.populateSaltedPassword();
    fs.watch(googlePlusFile, (_1, _2) => {
      this.populateGooglePlusProfile();
    });
    fs.watch(saltedPasswordFile, (_1, _2) => {
      this.populateSaltedPassword();
    });
  }

  private populateGooglePlusProfile() {
    const result: {[key: string]: string} = {};
    const pairs = readKeyValueFile(this.googlePlusFile);
    if (pairs === undefined) {
      return;
    }
    for (const [userId, googlePlusId] of pairs) {
      if (result[googlePlusId] !== undefined) {
        console.log(`Duplicate googlePlusId: ${googlePlusId}`);
        return;
      }
      result[googlePlusId] = userId;
    }
    this.googleProfileIdToUserId = result;
  }

  private populateSaltedPassword() {
    const result: {[key: string]: string} = {};
    const pairs = readKeyValueFile(this.saltedPasswordFile);
    if (pairs === undefined) {
      return;
    }
    for (const [userId, hash] of pairs) {
      if (result[userId] !== undefined) {
        console.log(`Duplicate userid: ${userId}`);
        return;
      }
      result[userId] = hash;
    }
    this.userIdToHash = result;
  }

  public tryGetUserIdFromGoogleProfileId(id: string) {
    return this.googleProfileIdToUserId[id];
  }

  public tryGetUserIdFromUsernamePassword(user: string, password: string) {
    const hash = this.userIdToHash[user];
    if (hash === undefined) {
      return undefined;
    }
    if (!bcrypt.compareSync(password, hash)) {
      return undefined;
    }
    return user;
  }
}
