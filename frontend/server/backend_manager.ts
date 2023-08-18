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

import {ServerProfile} from "./config/server_profile";
import * as net from "net";
import {Logger} from "../shared/logger";
import {Chunker} from "../shared/chunker";
import {CRequest} from "../shared/protocol/control/crequest";

export class BackendManager {
  private socket: net.Socket | undefined;
  private pending: string[];
  private readonly logger: Logger;

  constructor(serverProfile: ServerProfile, callerHandleFragment: (s: string) => void,
              private readonly _callerHandleError: (s: string) => void) {
    this.socket = undefined;
    this.pending = [];
    this.logger = new Logger(`Backend Manager`);

    this.logger.log(`Connecting to the backend.`);
    let bs: net.Socket;
    bs = net.createConnection(serverProfile.backendPort, serverProfile.backendHost,
      () => this.handleConnection(bs));
    bs.setEncoding("utf8");
    bs.on("data", callerHandleFragment);
    bs.on("error", (e: NodeJS.ErrnoException) => this.handleProblemFromBackend(e.toString()));
    bs.on("close", () => this.handleProblemFromBackend("close"));
  }

  private handleConnection(socket: net.Socket) {
    this.logger.log(`Backend connected!`);
    this.socket = socket;
    const p = this.pending;
    this.pending = [];
    for (const s of p) {
      this.sendFragment(s);
    }
  }

  private handleProblemFromBackend(reason: string) {
    this.logger.log(`Encountered a problem: ${reason}`);
    this.close();
    this._callerHandleError(reason);
  }

  sendCRequest(crequest : CRequest) {
    const json = crequest.toJson();
    const jsonText = JSON.stringify(json);
    const raw = Chunker.wrap(jsonText);
    this.sendFragment(raw);
  }

  sendFragment(rawText: string) {
    if (this.socket === undefined) {
      this.pending.push(rawText);
    } else {
      this.socket.write(rawText);
    }
  }

  close() {
    if (this.socket === undefined) {
      return;
    }
    this.socket.destroy();
    this.socket = undefined;
  }
}
