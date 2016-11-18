Captty
==============
A tty recorder


## Install

Checkout the repository and build:

    $ git clone git://github.com/frsyuki/captty.git
    $ cd captty
    $ ./autogen.sh
    
    $ ./configure
    $ make
    $ sudo make install


## Usage

Recording mode `captty r <file>.pty [command...]` starts a new shell or given command.

Replay mode `captty p <file>.pty` replays the recorded tty outputs. You can use following keys to control:

        g    rewind to start
        l    skip forward
        h    skip back
        k    speed up
        j    speed down
        =    reset speed
        ;    pause/restart

