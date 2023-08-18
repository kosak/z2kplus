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

import * as fs from "fs";

export function readKeyValueFile(filename: string) : [string, string][] | undefined {
    const result: [string, string][] = [];
    console.log(`Reading ${filename}`);
    const contents = fs.readFileSync(filename, "utf8");
    const lines = contents.split('\n');
    for (const line of lines) {
        const trimmed = line.trim();
        if (trimmed.length === 0) {
            continue;
        }
        if (trimmed[0] == '#') {
            // comment
            continue;
        }
        const index = line.indexOf(' ');
        if (index < 0) {
            console.log(`Bad line has no space: ${line}`);
            return undefined;
        }
        const key = line.substring(0, index);
        const value = line.substring(index + 1);
        result.push([key, value]);
    }
    return result;
}
