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

enum State { Initial, AwaitingVoices, Ready }

export class SpeechViewmodel {
    private state: State;
    private selectedVoice: SpeechSynthesisVoice | undefined;
    availableVoices: SpeechSynthesisVoice[];
    private pendingMessages: string[];
    private keepaliveTimer: any | undefined;

    constructor() {
        this.state = State.Initial;
        this.selectedVoice = undefined;
        this.availableVoices = [];
        this.pendingMessages = [];
        this.keepaliveTimer = undefined;
    }

    onShown() {
        this.maybeInit();
    }

    maybeInit() {
        if (this.state !== State.Initial) {
            return;
        }
        this.state = State.AwaitingVoices;
        setTimeout(() => {
            const synth = window.speechSynthesis;
            synth.onvoiceschanged = () => {
                const available = synth.getVoices();
                if (available === undefined || available.length === 0) {
                    console.log("Can't get voices");
                    return;
                }
                console.log("got voices");
                this.availableVoices = available;
                this.selectedVoice = available[0];
                this.state = State.Ready;
                this.maybeDrainQueue();
            };
        });
    }

    speak(text: string) {
        this.maybeInit();
        this.pendingMessages.push(text);
        this.maybeDrainQueue();
    }

    private maybeDrainQueue() {
        if (this.state !== State.Ready) {
            return;
        }
        const pms = this.pendingMessages;
        if (pms.length === 0) {
            return;
        }
        this.pendingMessages = [];
        for (const pm of pms) {
            const utterance = new SpeechSynthesisUtterance(pm);
            if (this.selectedVoice !== undefined) {
                console.log("selecting a voice");
                utterance.voice = this.selectedVoice;
            }
            console.log("speaking");
            window.speechSynthesis.speak(utterance);
        }
        this.maybeStartKeepaliveTimer();
    }

    private maybeStartKeepaliveTimer() {
        if (this.keepaliveTimer === undefined) {
            console.log("hi. setting a new timer");
            this.keepaliveTimer = setTimeout(() => this.keepalive(), 10 * 1000);
        }
    }

    private keepalive() {
        console.log("hi. timer fired");
        this.keepaliveTimer = undefined;
        const ss = window.speechSynthesis;
        if (ss.speaking) {
            console.log("Still speaking! resume! unbelievably terrible");
            // This snaps the system out of its torpor when an installed voice has timed out.
            // But, I'm told, does a "cancel" on Android.
            ss.pause();
            ss.resume();
            this.maybeStartKeepaliveTimer();
        } else {
            console.log("let it go");

        }
    }

    test() {
        this.speak("Welcome to Zarchive 2 K Plus");
    }
}
