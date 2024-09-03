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

import {MetadataRecord, RenderStyle, ZgramCore} from "../../shared/protocol/zephyrgram";
import {Z2kState} from "../z2kstate";
import {isLoggableInstance, Pair, qEncode as utilQEncode} from "../../shared/utility";
import {ZgramViewModel} from "./zgram_viewmodel";
import {DisplayedRenderStyle} from "../util/renderer";

enum Mode {Compose = "Compose", Reply = "Reply", Edit = "Edit"}

export class ZgramEditorViewModel {
    static forCompose(state: Z2kState, onSubmitOrClose: (result: ZgramEditorResult | undefined) => void) {
        const okToSubmit = () => true;
        return new ZgramEditorViewModel(state, Mode.Compose, "", "", false, onSubmitOrClose, okToSubmit);
    }

    static forReply(state: Z2kState, initialInstance: string, initialBody: string,
        onSubmitOrClose: (result: ZgramEditorResult | undefined) => void) {
        const okToSubmit = () => true;
        return new ZgramEditorViewModel(state, Mode.Reply, initialInstance, initialBody, false, onSubmitOrClose, okToSubmit);
    }

    static forEdit(state: Z2kState, initialZgram: ZgramViewModel,
        onSubmitOrClose: (result: ZgramEditorResult | undefined) => void) {
        const okToSubmit = (result: ZgramEditorResult) => {
            if (initialZgram.isLogged !== result.isLogged) {
                // Can't change logging category
                return false;
            }
            const mds = ZgramEditorViewModel.calcUpdates(initialZgram, result);
            // Only valid to submit if there is a change
            return mds.length !== 0;
        }
        const wantMdmj = initialZgram.rawRenderStyle === RenderStyle.MarkDeepMathAjax;
        return new ZgramEditorViewModel(state, Mode.Edit, initialZgram.instance, initialZgram.body,
            wantMdmj, onSubmitOrClose, okToSubmit);
    }

    static calcUpdates(initialZgram: ZgramViewModel, result: ZgramEditorResult) {
        const zgramId = initialZgram.zgramId;
        const iTrim = result.instance;
        const mdrs: MetadataRecord[] = [];
        if (iTrim !== initialZgram.instance ||
            result.body !== initialZgram.body ||
            result.renderStyle !== initialZgram.rawRenderStyle) {
            const zgc = new ZgramCore(iTrim, result.body, result.renderStyle);
            mdrs.push(MetadataRecord.createZgramRevision(zgramId, zgc));
        }
        return mdrs;
    }

    instance: string;
    body: string;
    wantMarkdeepMathJax: boolean;
    uploadStatus: string | undefined;
    selection: Pair<number, number>;

    private constructor(private readonly state: Z2kState,
        public readonly humanReadableMode: Mode,
        private readonly initialInstance: string,
        private readonly initialBody: string,
        private readonly initialWantMarkdeepMathJax: boolean,
        private readonly onSubmitOrClose: (result: ZgramEditorResult | undefined) => void,
        private readonly callerOkToSubmit: (result: ZgramEditorResult) => boolean) {
        this.reset();
    }

    private reset() {
        this.instance = this.initialInstance;
        this.body = this.initialBody;
        this.wantMarkdeepMathJax = this.initialWantMarkdeepMathJax;
        this.uploadStatus = undefined;
        this.selection = Pair.create(0, 0);
    }

    get isDirty() {
        return this.instance !== this.initialInstance ||
            this.body !== this.initialBody ||
            this.wantMarkdeepMathJax !== this.initialWantMarkdeepMathJax;
    }

    getZmojis() {
        const onClick = (text: string) => {
            this.body += text;
        };
        return this.state.getZmojis(onClick);
    }

    submit() {
        if (!this.okToSubmit) {
            // Double check ok to submit, in case we are called e.g. from a key binding.
            return;
        }
        const result = this.makeZgramEditorResult();
        this.reset();
        this.onSubmitOrClose(result);
    }

    close() {
        this.onSubmitOrClose(undefined);
    }

    guiReset() {
        if (!window.confirm("Reset: Are you sure?")) {
            return;
        }
        this.reset();
    }

    get bodyAsHtml() {
        const rs = this.wantMarkdeepMathJax ? DisplayedRenderStyle.MarkDeepMathAjax : DisplayedRenderStyle.Default;
        return this.state.renderer.renderBodyAsHtml(this.body, rs);
    }

    get logged() {
        return isLoggableInstance(this.instance.trim());
    }

    maybeUpdateMathJax(el: any) {
        if (this.wantMarkdeepMathJax) {
            this.state.renderer.updateMathJax(this.body, el);
        }
    }

    onKeyOrMouseUpForSelection(event: any) {
        const target = event.target;
        if (target === undefined) {
            return;
        }
        const ss = target.selectionStart;
        const se = target.selectionEnd;
        if (ss === undefined || se === undefined) {
            return;
        }
        this.selection = Pair.create(ss, se);
    }

    get qEncodeLabel() {
        return this.selection.first === this.selection.second ? "Q-encode" : "Q-encode selection";
    }

    qEncode() {
        // probably not necessary, but doesn't hurt either.
        let sel = this.clamp(this.selection);
        if (sel.first === sel.second) {
            sel = Pair.create(0, this.body.length);
        }
        let prefix = this.body.substring(0, sel.first);
        let encodable = this.body.substring(sel.first, sel.second);
        let suffix = this.body.substring(sel.second);

        let qEncoded = utilQEncode(encodable);
        this.body = prefix + qEncoded + suffix;
        // Selection gets messed up, and I don't know how to fix it. So, just forget it.
        this.selection = Pair.create(0, 0);
    }

    insertMarkdeepExamples() {
        this.wantMarkdeepMathJax = true;
        this.body += giantMarkdeepExample;
    }

    async pasteImage() {
        try {
            const [blob, suffix] = await this.state.uploadableMediaUtil.getClipboardBlob();
            if (blob === undefined) {
                this.uploadStatus = "Can't find GIF, JPEG, or PNG in clipboard";
                return;
            }

            this.uploadStatus = "Uploading...";

            const url = `https://${window.location.host}/uploadImage?suffix=${suffix}`;
            const response = await fetch(url, {
                method: "POST",
                body: blob
            });

            if (response.ok) {
                const imageUrl = await response.text();
                const markdeepMarkup = `![](${imageUrl} width=512)`;
                this.body += markdeepMarkup;
                this.wantMarkdeepMathJax = true;
                this.uploadStatus = "Uploaded";
            } else {
                this.uploadStatus = "Server error while uploading image";
            }
        } catch (error) {
            console.log(error);
            this.uploadStatus = `${error.name}: ${error.message}`;
        }
    }

    clamp(sel: Pair<number, number>) {
        const clampHelper = (n: number) => Math.max(0, Math.min(n, this.body.length));
        return Pair.create(clampHelper(sel.first), clampHelper(sel.second));
    }

    get okToSubmit() {
        if (this.instance.trim().length === 0 || this.body.trim().length === 0) {
            return false;
        }
        return this.callerOkToSubmit(this.makeZgramEditorResult());
    }

    private makeZgramEditorResult() {
        const rs = this.wantMarkdeepMathJax ? RenderStyle.MarkDeepMathAjax : RenderStyle.Default;
        return new ZgramEditorResult(this.instance.trim(), this.body, rs);
    }
}

export class ZgramEditorResult {
    constructor(readonly instance: string, readonly body: string, readonly renderStyle: RenderStyle) {}

    get isLogged() {
        return isLoggableInstance(this.instance);
    }
}

const giantMarkdeepExample = `
![Never gonna give you up](https://upload.wikimedia.org/wikipedia/commons/thumb/f/f9/Rick_Astley_Tivoli_Gardens.jpg/2880px-Rick_Astley_Tivoli_Gardens.jpg width=256)

~~~
bool have_given_you_up() {
  return false;
}
~~~

$ x^2 = 5 $

Who | When
----|------
You | Never
[When I will give up]

!!! WARNING
    Never gonna give you up
    
*******************
*     .--------.  *
*    /  You   /   *
*   '-+------'    *
*     |     ^     *
*     v     |     *
*     .     | no  *
*    / \\    |     *
*   /   \\   |     *
*  +Give +-'      *
*   \\Up?/         *
*    \\ /          *
*     +           *
*******************

    
<img src="http://g.gravizo.com/svg?digraph G { never -> give_up -> you; }">
`;
