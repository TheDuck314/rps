""" Very slightly biased toward one move. """
import random
import sys

def send_move(s):
    print(s)
    sys.stdout.flush()

def read_opponent_move():
    return sys.stdin.readline()[:1]

if __name__ == "__main__":
    bias = float(sys.argv[1])
    assert 0.0 <= bias <= 1.0

    MOVES = ["R", "P", "S"]
    favorite = random.choice(MOVES)
    while True:
        if random.random() < bias:
            send_move(favorite)
        else:
            send_move(random.choice(MOVES))
        read_opponent_move()