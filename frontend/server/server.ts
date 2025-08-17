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

// This is the main entry point for the node.js-based webserver that serves zarchive.

// Various stuff that comes with node.js
import * as crypto from "crypto";
import * as fs from "fs";  // file system utilities
import * as path from "path";
import * as http from "http";  // http serving
import * as https from "https";  // https serving
import * as url from "url";  // url parsing

// express.js (web application framework). Also body parsing and session middleware that works with
// express.
import * as express from "express";
import * as essc from "express-serve-static-core";
import * as expressWs from "express-ws";
import * as bodyParser from "body-parser";
import * as expressSession from "express-session";
import * as ws from "ws";

let sessionFileStore = require("session-file-store");
const FileStore = sessionFileStore(expressSession);
import {Express} from "express";

// passport.js (authentication middleware that works with express).
import * as passport from "passport";
import * as passportLocal from "passport-local";
import * as passportGoogleOauth from "passport-google-oauth";

// Argument parsing
// import * as argparse from "argparse";
import {ArgumentParser} from "argparse";

// import {IOAuth2StrategyOption} from "passport-google-oauth";

// my libraries
import {AuthorizationManager} from "./authorization_manager";
import {ClientManager} from "./client_manager";
import {passportUtil} from "./passportutil";

import {FileStructure} from "./config/file_structure";
import {ServerProfile} from "./config/server_profile";
import {UploadableMediaUtil} from "../shared/uploadable_media_util";

class Server {
  private readonly authorizationManager: AuthorizationManager;
  private readonly clientManager: ClientManager;

  constructor(private readonly serverProfile: ServerProfile) {
    this.authorizationManager = new AuthorizationManager(serverProfile.googleProfilesFile,
        serverProfile.saltedPasswordsFile);
    this.clientManager = new ClientManager(this.authorizationManager, serverProfile);
    this.makeHttpsServer();
    this.makeHttpServer();
    console.log(`Listening on HTTPS ${serverProfile.httpsPort} and redirecting` +
      ` HTTP ${serverProfile.httpPort} there.`);
  }

  // Our HTTP server. It has one job: redirecting all requests to our HTTPS server.
  private makeHttpServer() {
    console.log("No longer redirecting http");
    // const app = express();
    // app.all("*", (req, res) => {
    //   // Make a fake URL so we can parse it using the url library.
    //   const fakeUrl = `http://${req.headers["host"]}${req.url}`;
    //   const urlObj = url.parse(fakeUrl);
    //   urlObj.protocol = "https:";
    //   urlObj.host = null;  // Clear 'host' property so that 'hostname' property shows through.
    //   urlObj.port = this.serverProfile.httpsPort.toString();
    //   var redir = url.format(urlObj);
    //   console.log(`I'd like to redirect to ${redir}`);
    //   res.redirect(redir);
    // });
    //
    // http.createServer(app).listen(this._serverProfile.httpPort);
  }

  // Our HTTPS server. Serves:
  //   Our main entry point: "/"
  //   Our authentication endpoints: "/auth/*"
  //   Our logout endpoint: "/logout"
  //   Our static resources: "/static/chat.html and /static/login.html"
  //   Our static media resources like uploaded images: "/media"
  //   Our API endpoint: "/api"
  //
  // Various important pages:
  //   The main entry point: "/"
  //   The login page: "login.html" (configured to be served statically from our public/ directory)
  //   The chat page: "chat.html" (configured to be served statically from our public/ directory)
  //
  // Various sane things happen if the user is not authenticated. The most important thing is that the
  // websocket connection will be rejected if the user is not authenticated. So even if someone runs our
  // JavaScript (which is itself not a secret) that doesn't mean they get to chat. They may get our
  // default chat.html page which may look like it's doing something but it won't actually connect to
  // the chat server.
  private makeHttpsServer() {
    const privateKey = fs.readFileSync(this.serverProfile.privateKeyFile, "utf8");
    const certificate = fs.readFileSync(this.serverProfile.certificateFile, "utf8");
    const chain = fs.readFileSync(this.serverProfile.chainFile, "utf8");

    const clientId = fs.readFileSync(this.serverProfile.clientIdFile, "utf8").trim();
    const clientSecret = fs.readFileSync(this.serverProfile.clientSecretFile, "utf8").trim();

    const credentials = {key: privateKey, cert: certificate, ca: chain};
    const app = express();

    // Not much else works if express doesn't have the body parser middleware installed.
    app.use(bodyParser.urlencoded({extended: true}));

    // Persistent secret which survives restarts, but which is generated randomly the first time.
    const secret = this.getOrMakeSecret();
    // We use the following  middleware in two places: both for express and for socket.IO.
    // We do this because we want to reuse the same code and allow socket.IO to be able to ensure the
    // user is authenticated.
    const emw = expressSession({
      name: FileStructure.cookieName,
      cookie: {secure: true, maxAge: 30 * 24 * 60 * 60 * 1000}, // 30 days
      secret: secret,
      resave: false,
      saveUninitialized: false,
      store: new FileStore({
        path: this.serverProfile.clientSessionDir,
        ttl: 60 * 60 * 24
      })
    });
    app.use(emw);

    // Passport middlewares
    app.use(passport.initialize());
    app.use(passport.session());

    // Install some authentication strategies into passport.
    this.installPassportAuthenticationStrategies(app, clientId, clientSecret);

    // Middleware to serve static resources that are in the 'static' directory.
    // But divert any unauthenticated uses of chat.html back to the login page.
    app.get("/static/chat.html", (req, res, next) => {
      if (!passportUtil.isAuthenticated(req)) {
        return res.redirect("/static/login.html");
      }
      next();
    });
    app.use("/static", express.static("client/static"));
    app.use("/dist", express.static("client/dist"));

    // Middleware to serve media resources that are in the 'media' directory.
    // But divert any unauthenticated uses of chat.html back to the login page.
    app.use("/media", (req, res, next) => {
      if (!passportUtil.isAuthenticated(req)) {
        return res.redirect("/static/login.html");
      }
      next();
    });
    app.use("/media", express.static(this.serverProfile.mediaRoot));

    // Set up the socket that we use for asynchronous communication.
    const server = https.createServer(credentials, app);
    const ews = expressWs(app, server);
    const expressApp = ews.app;

    // Set up the routes.

    // The main page conditionally goes either to the chat page or the login page, depending on
    // whether the user is authenticated.
    app.get("/", (req: essc.Request, res: essc.Response) => {
      if (passportUtil.isAuthenticated(req)) {
        return res.redirect("/static/chat.html");
      }
      res.redirect("/static/login.html");
    });

    app.get("/logout", (req: essc.Request, res: essc.Response) => {
      if (req.session) {
        req.session.destroy(error => {
          console.log("Session destroy failed, error = ", error);
        });
      }
      // This prevents us from getting a bunch of warnings after logging out and back in.
      // See https://github.com/valery-barysok/session-file-store/issues/41#issuecomment-271133420
      res.clearCookie(FileStructure.cookieName);
      res.redirect("/static/login.html");
    });

    const uploadableMediaUtil = new UploadableMediaUtil();

    expressApp.post("/uploadImage", express.raw({type: "*/*", limit: "10mb"}), async (req: essc.Request, res: essc.Response) => {
      if (!passportUtil.isAuthenticated(req)) {
        console.log("post not authenticated, bye");
        return res.sendStatus(401);  // Unauthorized
      }

      const uuid = crypto.randomUUID();
      const suffix = req.query["suffix"];
      if (suffix === undefined) {
        console.log("No suffix parameter");
        return res.sendStatus(400);  // Bad request
      }
      const suffixStr = suffix.toString();
      if (!uploadableMediaUtil.isValidFileSuffix(suffixStr)) {
        console.log(`Suffix parameter ${suffixStr} not allowed`);
        return res.sendStatus(400);  // Bad request
      }

      const destName = `${uuid}.${suffixStr}`;
      const destPath = path.join(this.serverProfile.mediaRoot, destName);
      const destUrl = uploadableMediaUtil.makeVirtualMediaUrl(destName);

      const fsp = fs.promises;
      await fsp.writeFile(destPath, req.body);
      console.log(`Wrote file ${destPath}`);
      res.send(destUrl);
    });

    expressApp.ws("/api", (ws, req) => {
      const userId = passportUtil.tryGetAuthenticatedUser(req);
      if (userId === undefined) {
        const message = "Server rejected connection because user is not authorized.";
        console.log(message);
        ws.close(1008, message);
        return;
      }
      this.clientManager.newClient(ws, userId, "");
    });

    expressApp.ws("/oauth2api", (ws, req) => {
      this.clientManager.oauthClient(ws, clientId);
    });


    server.listen(this.serverProfile.httpsPort);
  }

// Install the various authentication strategies that we care about into passport.
  private installPassportAuthenticationStrategies(app: express.Express, clientId: string,
                                                  clientSecret: string) {
    // For now all we need are identity functions.
    passport.serializeUser((user, cb) => cb(null, user));
    passport.deserializeUser((id, cb) => cb(null, id as Express.User));

    // Local authentication: uses a salted password file.
    passport.use(new passportLocal.Strategy((username, password, cb) => {
      const user = this.authorizationManager.tryGetUserIdFromUsernamePassword(username, password);
      if (user === undefined) {
        return cb(null, false);
      }
      return cb(null, user);
    }));

    // Google authentication.
    // This clientID and clientSecret belong to kosak's Google Compute Engine project. We will need
    // to eventually protect them better. It's not completely horrible, because the credentials need
    // to match our callback anyway.
    const strategy = {
      clientID: clientId,
      clientSecret: clientSecret,
      callbackURL: this.serverProfile.callbackURL,
      userProfileURL: "https://www.googleapis.com/oauth2/v3/userinfo"
    };
    passport.use(new passportGoogleOauth.OAuth2Strategy(strategy,
      (accessToken, refreshToken, profile, done) => {
        // console.log(`accessToken`, accessToken);
        // console.log(`refreshToken`, refreshToken);
        // console.log(`profile`, profile);
        console.log(`I just got a login from id ${profile.id} name ${profile.displayName}`);
        const user = this.authorizationManager.tryGetUserIdFromGoogleProfileId(profile.id);
        if (!user) {
          return done(`Google user ${profile.id} not recognized`);
        }
        return done(null, user);
      }
    ));

    // Set up the routes that have to do with authentication.
    const redirects = {
      successRedirect: "/static/chat.html",
      failureRedirect: "/static/login.html"
    };

    // /auth/local implements our local authentication flow.
    app.post("/auth/local", passport.authenticate("local", redirects));

    // /auth/google and /auth/google/callback implement the Google authentication flow.
    app.get("/auth/google",
      passport.authenticate("google", {scope: ["profile"]}));
    app.get("/auth/google/callback",
      passport.authenticate("google", redirects));
  }

  private getOrMakeSecret(): string {
    const sessionDir = this.serverProfile.clientSessionDir;
    // ugh.
    try {
      fs.accessSync(sessionDir);
    } catch(e) {
      // owner: rwx
      fs.mkdirSync(sessionDir, {mode: 0o700});
    }

    const secretFile = path.join(sessionDir, "secret");
    try {
      fs.accessSync(secretFile);
    } catch(e) {
      const secret = crypto.randomBytes(256).toString('hex');
      // readonly by owner. I think this works, even though I'm writing it right now.
      fs.writeFileSync(secretFile, secret, {mode: 0o400});
    }

    return fs.readFileSync(secretFile, "utf8");
  }
}

function main(argv: string[]) {
  let parser = new ArgumentParser({
    description: "Z2KPlus+ front end",
    add_help: true,
  });
  parser.add_argument("--host", {help: "Fully-qualified name of this host"});
  parser.add_argument("--fileroot", {help: "Z2K files root (we need this for the media subdir)"});
  const result = parser.parse_args(argv);
  const hostToUse = result["host"];
  const fileRoot = result["fileroot"];
  if (hostToUse === undefined || fileRoot === undefined) {
    throw new Error("Please provide --host and --fileroot");
  }
  const profile = new ServerProfile(80, 443, "localhost", 8001, hostToUse, fileRoot);
  console.log(`Profile: `, profile);
  new Server(profile);
}

main(process.argv.slice(2));
