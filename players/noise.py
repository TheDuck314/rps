""" Plays randomly. """
import random
import sys

def send_move(s):
    print(s)
    sys.stdout.flush()

def read_opponent_move():
    return sys.stdin.readline()[:1]

if __name__ == "__main__":
    MOVES = ["R", "P", "S"]
    while True:
        send_move(random.choice(MOVES))
        read_opponent_move()
