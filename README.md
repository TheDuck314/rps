## rps

rps lets you play AIs against each other in the ancient strategy game of rock-paper-scissors:

    $ ./build.sh
    $ ./rps 'python3 ./players/beatlast.py' 'python3 ./players/patternmatcher.py'


### Game loop:
1. Write your move to standard output as one character ('R', 'P', or 'S') followed by a newline.
2. Read your opponent's move from standard input, in the same format.

