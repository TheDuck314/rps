""" Broken bot for testing the engine's error handling. """
import sys
import time

def send_move(s):
    print(s)
    sys.stdout.flush()

if __name__ == "__main__":
    instruction = sys.argv[1]
    if instruction == "hang":
        # hang after a few moves
        for _ in range(1000):
            send_move("R")
        while True:
            time.sleep(1)
    elif instruction == "invalid":
        # print an invalid move
        send_move("R")
        send_move("R")
        send_move("Q")
        while True:
            send_move("R")
    elif instruction == "die":
        # self destruct
        send_move("R")
        send_move("R")
        sys.exit(1)
    elif instruction == "exception":
        # self destruct
        send_move("R")
        send_move("R")
        raise Exception("something exceptional happened")
    else:
        raise Exception("unknown instruction '{}'".format(instruction))
