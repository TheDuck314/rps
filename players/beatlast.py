""" Play the move that would have beaten the opponent's last move. """
import random
import sys
import os

def send_move(s):
    print(s)
    sys.stdout.flush()

def read_opponent_move():
    return sys.stdin.readline()[:1]

if __name__ == "__main__":
    send_move(random.choice(["R", "P", "S"]))
    while True:
        opp_move = read_opponent_move()
        my_move = {
            "R": "P",
            "P": "S",
            "S": "R"
        }[opp_move]
        send_move(my_move)