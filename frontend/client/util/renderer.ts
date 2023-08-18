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

import {MathJaxUpdater} from "./mathjax_updater";
import {escape as utilEscape, linkify, staticFail, translateNewline} from "../../shared/utility"
import {Z2kState} from "../z2kstate";

export enum DisplayedRenderStyle {Default, Monospace, MarkDeepMathAjax}

export class Renderer {
    private readonly mathJaxUpdater = new MathJaxUpdater();

    constructor(private readonly state: Z2kState) {
    }

    renderBodyAsHtml(body: string, renderStyle: DisplayedRenderStyle) {
        switch (renderStyle) {
            case DisplayedRenderStyle.Default: {
                const escaped = utilEscape(body);
                const linkified = linkify(escaped);
                const newlined = translateNewline(linkified);
                return newlined;
            }
            case DisplayedRenderStyle.Monospace: {
                return `<pre>${body}</pre>`;
            }
            case DisplayedRenderStyle.MarkDeepMathAjax: {
                const md = (window as any).markdeep;
                if (!md || !md.format) {
                    return "(Renderer error: can't find window.markdeep.format)";
                }
                const withFixedMediaLinks =
                    this.state.uploadableMediaUtil.translateVirtualMediaUrlsToPhysical(body, this.state.host);
                const html = md.format(withFixedMediaLinks);
                return `<markdeep>${html}</markdeep>`;
            }
            default: staticFail(renderStyle);
        }
    }

    updateMathJax(body: string, element: any) {
        this.mathJaxUpdater.queueUpdateFor(element);
    }
}
