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
import {ServerProfile} from "./config/server_profile";
import {ProfileManager} from "./profile_manager";
import {BackendManager} from "./backend_manager";
import {Logger} from "../shared/logger";
import {CRequest} from "../shared/protocol/control/crequest";
import * as ws from "ws";

export class ClientManager {
  private readonly profileManager: ProfileManager;
  private nextFreeClientId: number;

  constructor(private readonly serverProfile: ServerProfile) {
    this.profileManager = new ProfileManager(serverProfile.z2kPlusProfilesFile);
    this.nextFreeClientId = 0;
  }

  newClient(clientSocket: ws.WebSocket, userId: string) {
    const profile = this.profileManager.tryGetProfile(userId);
    if (profile === undefined) {
      console.log(`ClientManager can't find profile for ${userId}. Something is broken.`);
      clientSocket.close();
      return;
    }

    const id = this.nextFreeClientId++;
    console.log(`ClientManager making new Client handler ${id} for userid ${userId}`);
    new Client(id, this.serverProfile, clientSocket, profile);
    // I don't *think* I need to keep the Client variable around to avoid GC, do I?
  }
}

// We don't do much except forward messages to and from the backend.
class Client {
  constructor(
    private readonly _id: number,
    serverProfile: ServerProfile,
    private readonly _socket: ws.WebSocket,
    profile: Profile) {

    this._logger = new Logger(`Client ${_id}`);
    this._backendManager = new BackendManager(serverProfile, s => this.handleFragmentFromBackend(s),
        s => this.handleCloseFromBackend(s));

    // Send a Hello to the backend
    const hello = CRequest.createHello(profile);
    this._backendManager.sendCRequest(hello);

    const s = this._socket;
    s.on("message", (data, isBinary) => this.handleMessageFromFrontend(data, isBinary));
    s.on("error", () => this.handleCloseFromFrontend(`error`));
    s.on("close", () => this.handleCloseFromFrontend(`close`));

    // Terminate the client after 10 seconds (this is for debugging)
    // setTimeout(() => {
    //   const message = "Forcibly disconnecting for debugging purposes";
    //   this._logger.log(message);
    //   s.close(1008, message);
    // }, 10 * 1000);
  }

  private handleMessageFromFrontend(data: ws.RawData, isBinary: boolean) {
    this._logger.log(`Got frontend message`, data);
    console.log(data);
    const message = data.toString();
    this._backendManager.sendFragment(message);
  }

  private handleCloseFromFrontend(reason: string) {
    this._logger.log(`Got frontend close ${reason}`);
    this.close();
  }

  private handleFragmentFromBackend(msg: string) {
    this._logger.log(`Got backend fragment ${msg}`);
    this._socket.send(msg);
  }

  private handleCloseFromBackend(reason: string) {
    this._logger.log(`Got backend close ${reason}`);
    this.close();
  }

  private close() {
    this._backendManager.close();
    this._socket.close(1000);
  }

  private readonly _logger: Logger;
  private readonly _backendManager: BackendManager;
}
