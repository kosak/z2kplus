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

// MIME type -> filename suffix
import {magicConstants} from "./magic_constants";

export class UploadableMediaUtil {
    private readonly mimeTypeToSuffix: { [key: string]: string } = {
        "image/gif": "gif",
        "image/jpeg": "jpg",
        "image/png": "png"
    };
    private readonly suffixToMimeType: { [key: string]: string } = {};

    constructor() {
        for (const [mimeType, s] of Object.entries(this.mimeTypeToSuffix)) {
            this.suffixToMimeType[s] = mimeType;
        }
    }

    async getClipboardText() : Promise<string | undefined> {
        try {
            return await navigator.clipboard.readText();
        } catch {
            return undefined;
        }
    }

    async getClipboardBlob() : Promise<[Blob | undefined, string | undefined]> {
        const clipboardItems = await navigator.clipboard.read();
        for (const citem of clipboardItems) {
            for (const ctype of citem.types) {
                const extension = this.tryGetFileSuffix(ctype);
                if (extension === undefined) {
                    continue;
                }
                const blob = await citem.getType(ctype);
                return [blob, extension];
            }
        }
        return [undefined, undefined];
    }

    tryGetFileSuffix(mimeType: string) : string | undefined {
        return this.mimeTypeToSuffix[mimeType];
    }

    isValidFileSuffix(suffix: string) : boolean {
        return this.suffixToMimeType[suffix] !== undefined;
    }

    makeVirtualMediaUrl(route: string) {
        return magicConstants.z2kVirtualMediaPrefix + route;
    }


    translateVirtualMediaUrlsToPhysical(body: string, host: string) : string {
        const physicalMediaPrefix = `https://${host}/media/`;
        // Could use string.replaceAll if we depend on until ES2021 library.
        const splits = body.split(magicConstants.z2kVirtualMediaPrefix);
        return splits.join(physicalMediaPrefix);
    }
}
