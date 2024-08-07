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

import {OAuth2Client, VerifyIdTokenOptions} from "google-auth-library";
import {Profile} from "../shared/protocol/profile";
import {ServerProfile} from "./config/server_profile";
import {ProfileManager} from "./profile_manager";
import {BackendManager} from "./backend_manager";
import {Logger} from "../shared/logger";
import {CRequest, crequestInfo, crequests} from "../shared/protocol/control/crequest";
import * as ws from "ws";
import {Chunker} from "../shared/chunker";
import {assertArrayOfLength, assertString} from "../shared/json_util";
import {LoginTicket} from "google-auth-library/build/src/auth/loginticket";
import {AuthorizationManager} from "./authorization_manager";

export class ClientManager {
  private readonly profileManager: ProfileManager;
  private nextFreeClientId: number;

  constructor(readonly authorizationManager: AuthorizationManager,
      private readonly serverProfile: ServerProfile) {
    this.profileManager = new ProfileManager(serverProfile.z2kPlusProfilesFile);
    this.nextFreeClientId = 0;
  }

  newClient(clientSocket: ws.WebSocket, userId: string, residual: string) {
    const profile = this.profileManager.tryGetProfile(userId);
    if (profile === undefined) {
      console.log(`ClientManager can't find profile for ${userId}. Something is broken.`);
      clientSocket.close();
      return;
    }

    const id = this.nextFreeClientId++;
    console.log(`ClientManager making new Client handler ${id} for userid ${userId}`);
    new Client(id, this.serverProfile, clientSocket, profile, residual);
  }

  oauthClient(clientSocket: ws.WebSocket, oauthClientId: string) {
    const id = this.nextFreeClientId++;
    console.log(`ClientManager making new OauthClient handler ${id}`);
    new WaitForOauthToken(this, id, clientSocket, oauthClientId);
  }
}

class WaitForOauthToken {
  private readonly _logger: Logger;
  private readonly _chunker: Chunker;
  private _receivedFirstMessage: boolean;
  private readonly _messageHandler: (data: ws.RawData, isBinary: boolean) => void;
  private readonly _errorHandler: (reason: string) => void;
  private readonly _closeHandler: (reason: string) => void;

  constructor(private readonly _owner: ClientManager, private readonly id: number,
      private readonly _socket: ws.WebSocket, private readonly _oauthClientId: string) {
    this._logger = new Logger(`OAuthClient ${id}`);
    this._chunker = new Chunker();
    this._receivedFirstMessage = false;
    // painful
    this._messageHandler = (data, isBinary) => this.handleMessageFromFrontend(data, isBinary);
    this._errorHandler = () => this.handleCloseFromFrontend(`error`);
    this._closeHandler = () => this.handleCloseFromFrontend(`close`);

    const s = this._socket;
    s.on("message", this._messageHandler);
    s.on("error", this._errorHandler);
    s.on("close", this._closeHandler);
  }

  private handleMessageFromFrontend(data: ws.RawData, isBinary: boolean) {
    this._logger.log(`Got next frontend fragment`, data);
    const message = data.toString();
    this._chunker.pushBack(message);

    if (this._receivedFirstMessage) {
      this._logger.log(`The first message is still being processed. Caching this fragment:`, data);
      return;
    }

    const firstMessageText = this._chunker.tryUnwrapNext();
    if (firstMessageText === undefined) {
      return;
    }

    this._receivedFirstMessage = true;
    this._logger.log(`Got full first message. Processing.`, firstMessageText);

    try {
      const firstMessageValue = JSON.parse(firstMessageText);
      const creq = CRequest.tryParseJson(firstMessageValue);
      if (creq.tag !== crequestInfo.Tag.Auth) {
        throw new Error(`Expected first message to be an Auth message, got ${creq.tag}`);
      }
      this.verifyId((creq.payload as crequests.Auth).token);
    } catch (e) {
      this._logger.log("Caught exception", e);
      this.close();
      return;
    }
  }

  private handleCloseFromFrontend(reason: string) {
    this._logger.log(`Got frontend close ${reason}`);
    this.close();
  }

  private handleCloseFromBackend(reason: string) {
    this._logger.log(`Got backend close ${reason}`);
    this.close();
  }

  private close() {
    this._socket.close(1000);
  }

  private verifyId(idToken: string) {
    if (idToken.startsWith("HACK:")) {
      this.proceedWithUser(idToken.substring(5));
      return;
    }
    const oa2c = new OAuth2Client();
    const options = {
      idToken: idToken,
      audience: this._oauthClientId,  // Specify the CLIENT_ID of the app that accesses the backend
      // Or, if multiple clients access the backend:
      //[CLIENT_ID_1, CLIENT_ID_2, CLIENT_ID_3]
    } as VerifyIdTokenOptions;
    oa2c.verifyIdToken(options, (err: Error | null, login?: LoginTicket) => this.verifyIdCallback(err, login));
  }

  private verifyIdCallback(err: Error | null, login?: LoginTicket) {
    if (err !== null) {
      this._logger.log(`Error from verifyIdToken: ${err.message}`);
      this.close();
      return;
    }
    this._logger.log(`verifyIdToken seemingly successful`);
    const payload = login!.getPayload();
    const profileId = payload!['sub'] as string;
    this._logger.log(`profileId is ${profileId}`);

    const userId = this._owner.authorizationManager.tryGetUserIdFromGoogleProfileId(profileId);
    if (userId === undefined) {
      this._logger.log(`Can't find corresponding userId`);
      this.close();
      return;
    }
    this.proceedWithUser(userId);
  }

  private proceedWithUser(userId: string) {
    this._logger.log(`I think userId is ${userId}`);
    const s = this._socket;
    s.off("message", this._messageHandler);
    s.off("error", this._errorHandler);
    s.off("close", this._closeHandler);

    const residual = this._chunker.residual;
    this._owner.newClient(this._socket, userId, residual);
  }
}

// We don't do much except forward messages to and from the backend.
class Client {
  constructor(
    id: number,
    serverProfile: ServerProfile,
    private readonly _socket: ws.WebSocket,
    profile: Profile,
    residual: string) {

    this._logger = new Logger(`Client ${id}`);
    this._backendManager = new BackendManager(serverProfile, s => this.handleFragmentFromBackend(s),
        s => this.handleCloseFromBackend(s));

    // Send a Hello to the backend
    const hello = CRequest.createHello(profile);
    this._backendManager.sendCRequest(hello);

    // Send residual fragments, if any, to the backend
    this._backendManager.sendFragment(residual);

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
