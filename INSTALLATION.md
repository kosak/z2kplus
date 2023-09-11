# Installation

## Background

Eventually we should make a Dockerfile for this. But for now, these are the
steps for setting up Z2K Plus+ on a Google Cloud VM.

1. Pick a region and zone.
   * I've been using us-east1 and us-east1b.
   * Sort of doesn't matter too much. You might care a little about latency.
     I might otherwise choose us-east4 but us-east1 is cheaper.

2. Reserve a static IPv4 address on Google Cloud
   * console.cloud.google.com
   * VPC network -> ip addresses -> reserve external ip address
   * I chose Network Service Tier Premium, IPv4, and Regional

3. Pick a name for your machine (e.g. z2k.plus) and configure your DNS settings
   (whoever your DNS provider is) to point to the IP address allocated by
   Google above.
   * I recommend keeping DDNS TTL as low as you can at first, until you are
     sure your DNS settings are right.

4. Create a VM on Google Cloud
   * console.cloud.google.com
   * compute engine -> VM instances -> CREATE INSTANCE
     * n2-standard-4
       * -4 might be overkill. Might get away with -2.
     * 60G disk (Balanced persistent disk)
     * Ubuntu 22.04 x86/64
     * set network to have the external IP address you reserved above
     * click the checkbox boxes to open the HTTP and HTTPS ports
       * The only reason you need HTTP is to interface with letsencrypt.
         So you could theoretically only need to open that port whenever
	 you create or refresh your certificates. Whatever.

5. ssh to the machine, using your public DNS name

6. Install some dependencies
   ```
   sudo apt-get update
   sudo apt-get install curl xauth git gitk git-gui emacs gcc g++ cmake pkg-config manpages-dev uuid-dev python3-pip default-jdk certbot fonts-inconsolata silversearcher-ag terminator
   ```
7. Get a SSL key from letsencrypt
    * Make sure that ports 80 and 443 are open (you should have clicked the
      little checkboxes when you made the VM)
    ```
    sudo certbot certonly
    Option 1 for temporary webserver

8. Configure your VM to have swap (I like having lots of swap around)
   ```
   sudo fallocate -l 16G /mnt/swapfile
   sudo chmod 600 /mnt/swapfile
   sudo mkswap /mnt/swapfile
   sudo swapon /mnt/swapfile
   echo '/mnt/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
   ```

9. [optional] Install xpra
   * https://github.com/Xpra-org/xpra/wiki/Download#-linux
   * to deal with my 3-wide 4K monitors, I edited /etc/xpra/conf.d/55_server_x11.conf
     * Changing -dpi 96x96 to -dpi 144x144
     * Changing the width of the virtual frame buffer to 11520x2160x24+32
       (that's the 4K resolution 3840x2160 times 3 makes 11520x2160)

10. Install nodejs
    * https://nodejs.org/en/download/package-manager/#debian-and-ubuntu-based-linux-distributions
    * Click through Node.js binary distributions
    * Click through to here: https://github.com/nodesource/distributions#ubuntu-versions
    * The current instructions are:
      ```
      sudo apt-get update
      sudo apt-get install -y ca-certificates curl gnupg
      sudo mkdir -p /etc/apt/keyrings
      curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key | sudo gpg --dearmor -o /etc/apt/keyrings/nodesource.gpg
      NODE_MAJOR=18
      echo "deb [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_$NODE_MAJOR.x nodistro main" | sudo tee /etc/apt/sources.list.d/nodesource.list
      sudo apt-get update
      sudo apt-get install nodejs -y
      ```
    * We are using Node 18 but someday we can upgrade to Node 20 I suppose.
    * Confirm node is alive and well
      ```
      % node
      > 3+3
      6
      ```

11. Install nodemon, webpack, and typescript
    * What are they?
      * Nodemon: a little program that watches for changes in the file system
        and then restarts nodejs to restart your program.
        * Not actually necessary for production.
      * webpack : a program that crunches a bunch of JavaScript files down into
         one so you can slurp them over to a browser. It also understands
         TypeScript so it can do TypeScript->JavaScript->crunch
      * TypeScript: Microsoft’s fabulous web language that is kind of like
        JavaScript but more disciplined. It compiles down to JavaScript.
    ```
    sudo npm install -g nodemon webpack webpack-cli typescript ts-loader npm-check-updates
    ```
    * npm is the "node package manager"
    * "-g" means "global"... i.e. install these things in some global location
      rather than in the node_modules subdirectory of wherever you happen to
      be running from. (The default behavior of npm is to make private copies
      of everything and stuff them in the subdirectory called node_modules.
      Which makes sense for a lot of things but not for generally useful
      utilities like nodemon/webpack/typescript)

12. Build ANTLR4 from source
    * The C++ backend needs the ANTLR4 runtime in order to run. You will also
      need to run the ANTLR4 compiler if you ever recompile the parser/lexer
      files. The ANTLR website is www.antlr.org
      ```
      cd $HOME
      git clone https://github.com/antlr/antlr4.git
      cd antlr4
      git checkout -b zamboni v4.11.1
      cd runtime/Cpp
      # There is a sad little fix that needs to be made
      # In the file runtime/src/atn/ATNState.h,
      # find the line that says "class ANTL4CPP_PUBLIC ATN;"
      # and change it to "class ATN;"
      # But make sure you end up back in antlr4/runtime/Cpp
      mkdir build && mkdir run && cd build
      cmake ..
      make -j4  # -jN is to put some parallelism into make
      DESTDIR=../run make install
      cd ../run/usr/local
      sudo cp -r * /usr/local   # maybe there is a better way to do this

13. You probably have to run 'ldconfig' to rebuild the shared library database:
    ```
    sudo ldconfig
    ```

14. Install the ANTLR tools like this
    ```
    pip install antlr4-tools
    # maybe arrange to put the below in your .bashrc
    export PATH=$USER/.local/bin:$PATH
    ```
    * You will need this for grammar recompilation
    * You should have enough now to run the ANTLR4 test example (graphically)
      if you want.

15. Clone the code repo, the secrets repo, the plaintext repo, and the
    media files.

    Pro Tip: I'm seeing this bug lately where git will just hang if you're not
    authenticated. I'm not sure why. So make sure you're forwarding your SSH
    keys properly or whatever.

    ```
    cd ~ && mkdir git && cd git
    # The code repo
    git clone https://github.com/kosak/z2kplus.git
    # The secrets repo... you will need to be permissioned for this
    git clone git@github.com:kosak/z2k-secrets.git
    # The plaintext repo... you will also need to be permissioned for this
    # NOTE: replace "kosak@kosak.com" with your email that you've registered
    # with Google Source repositories and that I've permissioned for you.
    # Yes, the two at-signs are correct.
    git clone ssh://kosak@kosak.com@source.developers.google.com:2022/p/zarchive2000/r/plaintext
    # The media. Note: gsutil is already installed on this machine.
    # I don't *think* you need to authenticate to gcloud
    # (if you did, you'd use "gcloud auth login" or we'd set up a service
    # account)
    cd plaintext
    mkdir media && cd media
    gsutil -m rsync -n -r gs://z2kplusmedia/media .  # dryrun
    gsutil -m rsync -r gs://z2kplusmedia/media .     # actual
    ```

16. Make the secrets directory

    ```
    # the final part of this is your hostname, e.g. z2k.plus or
    # debug.debuggery.com
    sudo mkdir -p /etc/z2kplus+/z2k.plus
    cd ~/git/z2k-secrets
    # Again, the final part of this is your hostname
    sudo cp * /etc/z2kplus+/z2k.plus
    # oops, fix permissions
    sudo bash
    chmod 0700 /etc/z2kplus+
    chmod 0700 /etc/z2kplus+/debug.debuggery.com
    chmod 0600 /etc/z2kplus+/debug.debuggery.com/*
    exit
    ```

17. You should now delete ~/git/z2k-secrets because you've copied its secrets

17. Get IAM credentials from Google so that the Google OAuth2 flow can work
    * console.cloud.google.com -> APIS & Services -> Credentials
    * CREATE CREDENTIALS -> OAuth client ID
    * Application type: Web application
    * Name: whatever
    * Authorized JavaScript origins:
    ```
    # or whatever your host is
    https://debug.debuggery.com
    ```
    # Authorized redirect URIs
    ```
    https://debug.debuggery.com/auth/google/callback
    ```
    * press CREATE.
    * Capture the Client ID text and store it in the file
    ```
    /etc/z2kplus+/debug.debuggery.com/client_id.txt
    ```
    With the correct host of course, and with permissions set to read only
    by root.

    * Capture the Client secret text and store it in the file
    ```
    /etc/z2kplus+/debug.debuggery.com/client_secret.txt
    ```
    with the same restrictions.

    You don't need to stash these anywhere else. The id and secret will be
    available on this Google cloud page if you come back to it.

18. Build the C++ backend
    ```
    cd ~/git/z2kplus/backend
    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
    make -j4 backend   # 4 == equal to the number of cores you have.
    ```
    We will eventually run the "backend" executable from this very spot.

19. Build the Typescript frontend
    ```
    cd ~/git/z2kplus/frontend
    npm install   # this syncs all the packages
    cd server
    tsc  # builds the node.js based webserver
    cd ../client
    webpack  # builds the code that runs in the browser
    ```

20. RUN EVERYTHING
    * I do this in a pretty low-tech way. I open a screen session that has
      two terminals inside it, and I leave them up forever.
    * OR, I open an xpra session with two terminals inside it and leave that
      up forever.
    * Inside terminal 1:
      ```
      cd ~/git/z2kplus/backend/build
      ./backend /home/kosak/git/plaintext
      ```
      This will begin to output a bunch of jibber-jabber, calls to
      /usr/bin/sort, etc, as it builds the index for the first time.
      It might take a couple of minutes before you see this message:
      ```
      Server is running. Enter STOP to stop.
      ```

      The backend will use the "logged" and "media" directories from
      git/plaintext and also create "unlogged", "scratch", and "index".
      The .gitignore file is configured to the directories other than "logged".
    * Inside terminal 2:
      ```
      cd ~/git/z2kplus/frontend
      # as usual, change debug.debuggery.com to whatever you are using
      sudo node server/generated/server/server.js --host=debug.debuggery.com --fileroot=/home/kosak/git/plaintext
      ```

21. Connect to your test instance from your web browser.
    ```
    https://debug.debuggery.com/
    ```
