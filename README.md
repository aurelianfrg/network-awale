# network-awale - A Client Server Awale game in C (with sockets)

*Authors :* DESNOT Anatole, FRANGIN Aurélian


## Features

Features implemented consist in : 
- A single-thread multi-connection server with extensive logging
- Multiple simultaneous independant games. 
- A client using a pretty terminal user interface (TUI) made from a personnalized library
- Register with a username 
- List every connected players
- Send a game invite to any connected user, who may accept or decline. Acceptance results in game initialization.
- Play a move in a started game.
- Chat with the opponent during a game.
- Disconnection of an user results in termination of any game or game invite, allowing the opponenent to start other games.
- Spectate any game, interact in chat as spectator.

## Implementation

#### Client interface
The client interface is a custom Terminal User Interface library made specifically for this project. It is located in the `tui.c` and `tui.h` files. It uses escape sequences to send commands, style, and text to the terminal. It supports bold, italic, underline, faint, inverse, strikethrough, background & foreground color (256 colors), any UTF-8 character, and uses a custom markup language for styling the text. The interface supports zooming and resizing (listening the `SIGWINCH` signal). 

The frame is drawn using a custom datastructure optimized for size: the *GridCharBuffer* (or gcbuf). It divides the terminal in a grid. Each terminal cell has a background & foreground color, a 4-byte buffer for any Unicode char, and 8 style flags (1 bit each). The grid is filled by coordinate (like pixels). At the end a the frame, an algorithms computes the minimum buffer size needed to print this frame, mallocs a char buffer of the right size, fills it and flushes it to the terminal in a single write call.

Many functions have been made to abstract the complexity of the *GridCharBuffer*. They are named `drawXXX`. They allow for responsive positioning (with anchor points like center, top-left, bottom-center, etc...), offsetting, and custom style. Any text can use a custom-made markup language for inline styling.

#### Communication protocol
The entire communication protocol is specified in the communication package (`communication.c` and `communication.h`).

Each message is sent in a single TCP package. They consist of a 32 bit integer header, indicating the message type. And a body of which size depends on the message type. Agreement between client and server is guaranteed by the common *communication.h* header. To parse an incoming message, the programs reads the 32 first bits of the recieved data to get the type and interprets the following bytes depending on this information.

Inside the server and clients, communication is handled with active polling, allowing for always-responsive single-thread programs. Everything is placed in an event-loop using `poll` on the sockets file descriptor and stdin. 

## How to run

You must have `make` and `gcc` in order to compile the code. The programs have only been tested on *Linux* and *WSL*, there may be errors when using other environments.

The two compilation targets are `server` and `client`. Thus, you must run `make server` and `make client`.

The binaries are placed in `./bin`. In order to run them, you can navigate to that directory or run them from their path. 

`server` takes one argument: the port to bind (*ex. 5050*). \
`client` takes two arguments: the server ip (*ex. 127.0.0.1*). \
The server must be run before the client.

```bash
# At project root
make server && make client
./bin/server 5050

# In a second terminal, at project root
./bin/client 127.0.0.1 5050
```

## AI usage

Over the course of the project, we used generative AI with different purposes : 
- To write the initial client/server structure using active polling. We had trouble using the given client/server example as many functions raised deprecation warnings and errors. So we chose to go with something more modern that we could understand better.
- To write the `u_charlen()` function in the `tui.c` to handle the low-level byte manipulation of the unicode characters.
- To debug a few bugs that (despite many efforts), we could not resolve ourselves.

## Misceallenous sources 

The TUI library was made possible thanks to the part 2 and 3 of the [Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/index.html) blog. 