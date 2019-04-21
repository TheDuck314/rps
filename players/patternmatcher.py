""" Track what move the opponent usually makes in each context, and
play the move that will counter what we expect them to play. """
import random
import sys
import os
from collections import defaultdict

VALID_MOVES = ["R", "P", "S"]
COUNTERS = {
    "R": "P",
    "P": "S",
    "S": "R"
}

if __name__ == "__main__":   
    my_moves = []   # list of all my moves
    opp_moves = []  # list of all opponent moves

    def send_move(s):
        my_moves.append(s)
        print(s)
        sys.stdout.flush()

    def read_opponent_move():
        s = sys.stdin.readline()[:1]
        opp_moves.append(s)
        return s

    # Move randomly for a few moves
    hist_len = 4
    for _ in range(hist_len):
        send_move(random.choice(VALID_MOVES))
        read_opponent_move()    

    opp_move_counts = defaultdict(lambda: 0)

    while True:
        # Track how often the opponent makes each move in each context
        context = tuple(my_moves[-hist_len:]) + tuple(opp_moves[-hist_len:])

        most_likely_opp_move = max(VALID_MOVES, key=lambda m: opp_move_counts[context, m])
        my_move = COUNTERS[most_likely_opp_move]
        send_move(my_move)

        # see what they actually did
        opp_move = read_opponent_move()
        opp_move_counts[(context, opp_move)] += 1
