""" Play one move and stick with it. """
import random
import sys
import os

def send_move(s):
    print(s)
    sys.stdout.flush()

def read_opponent_move():
    return sys.stdin.readline()[:1]

if __name__ == "__main__":
    # I read on the internet that random seeds itself from urandom
    move = random.choice(["R", "P", "S"])
    while True:
        send_move(move)
        read_opponent_move()