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

// A convenient place to put certain server parameters I care about.
import {FileStructure} from "./file_structure";

export class ServerProfile {
  constructor(readonly httpPort: number, readonly httpsPort: number, readonly backendHost: string,
    readonly backendPort: number, private server: string, private z2kRoot: string) {
  }

  // From LetsEncrypt
  get privateKeyFile() {
    return FileStructure.sslcertDirBase + this.server + "/privkey.pem";
  }

  // From LetsEncrypt
  get certificateFile() {
    return FileStructure.sslcertDirBase + this.server + "/cert.pem";
  }

  // From LetsEncrypt
  get chainFile() {
    return FileStructure.sslcertDirBase + this.server + "/chain.pem";
  }

  // From Google Cloud Platform: allocate an "OAuth2.0 Client ID" and store it here.
  get clientIdFile() {
    return FileStructure.clientSecretsDirBase + this.server + "/client_id.txt";
  }

  // From Google Cloud Platform: allocate an "OAuth2.0 Client ID" and store it here.
  get clientSecretFile() {
    return FileStructure.clientSecretsDirBase + this.server + "/client_secret.txt";
  }

  get clientSessionDir() {
    return FileStructure.clientSecretsDirBase + this.server + "/sessions";
  }

  get z2kPlusProfilesFile() {
    return FileStructure.clientSecretsDirBase + this.server + "/z2k_plus_profiles.txt";
  }

  get googleProfilesFile() {
    return FileStructure.clientSecretsDirBase + this.server + "/google_plus_ids.txt";
  }

  get saltedPasswordsFile() {
    return FileStructure.clientSecretsDirBase + this.server + "/salted_passwords.txt";
  }

  get mediaRoot() {
    return this.z2kRoot + "/media";
  }

  get callbackURL() {
    return "https://" + this.server + "/auth/google/callback";
  }
}
