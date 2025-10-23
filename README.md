# network-awale - A Client Server Awale game in C (with sockets)
## Objectives

The goal of this project is to implement an Awalé game server. The aim is to have a client/server application that allows clients to play matches, verify that the rules are correctly applied, communicate, and keep traces (scores, games played, etc.).

The project is incremental, you can add features to your application as your implementation progresses, spectating a game, organizing tournaments, group chats, logging conversations, end-to-end encryption. The only limits are your imagination and the time you dedicate.

Your work will be evaluated with a demonstration (plan ~10 minutes) and with the code you submit during the last session. Prioritize a robust application that works rather than many non-working features. Code scalability/extensibility is also evaluated, is it easy to add new features?

## Description of the expected work

You will implement your solution incrementally. It is essential to think carefully about architectural aspects before you start coding, what happens on the server? on the clients? what is your communication policy? What are the different protocols, for matches, for chats, etc.? How can clients call/challenge one another (by username only? otherwise?)? What choices ensure your code remains scalable or extensible? Application testing is important, what happens if a client disconnects? Can they recover the game? etc.

You can start from the example given in Lecture 1 where the server re-broadcasts to all clients a message sent by one client, [Client Server V2 in Examples, Lecture 1](https://moodle.insa-lyon.fr/mod/resource/view.php?id=203926)

## The Awalé game

The rules of Awalé are simple and can be found here, Wikipedia article Awalé.

You must implement this game, represent the board, check move legality, and determine the winner. You should also be able to save and broadcast match sessions, to players and, for example, to spectators. A simple ASCII output is sufficient (a nice GUI could be built on top, but that is not the goal of this project).

## Implementation guideline

0. Implement the Awalé game: internal representation, how to play a move, count points, save a game, print the board state, etc.
1. Design a client/server application. Each client registers with a username.
2. Each client can request from the server the list of online usernames.
3. Client A can challenge client B. B can accept or refuse.
4. When a match is created between A and B, the server randomly decides who starts. The server verifies the legality of moves (using the code created in step 0).
5. If you have a working version for one game, ensure it also works for multiple simultaneous games. You can add features such as listing ongoing games and an “observer” mode where the server sends the board and score to C who observes the game between A and B.
6. Implement a chat option, in addition to sending moves, players can exchange messages to chat (both inside and outside a game).
7. Allow a player to write a bio, say 10 ASCII lines to introduce themselves. The server should be able to display the bio of a given username.
7. Add a private mode, a player can limit the list of spectators to a list of friends. Implement this feature.
9. Add the ability to save played games so they can be reviewed later.
10. Free to your imagination, player rankings (Wikipedia article on Elo rating), tournament organization, adapting it to another game, etc.

## Programming environment

The program will be written in C. Distribute your functions across multiple files and provide a Makefile for compilation. Carefully choose your data structures. You can start from the examples seen in previous TPs or in class. The goal is to create a server that allows two clients to play a game of Awalé. The server checks move validity and counts points.

Start from an existing client-server base (from the previous TP or by reusing the example mentioned above).

Submit all the source files necessary for compilation along with instructions on how to compile. Also include a user manual (which features are implemented and how to use them for both the client and the server). Send everything by email to frederic.prost@insa-lyon.fr with the subject line [4IF-PR-TP-CR].

A 10-minute presentation is expected during the last session.