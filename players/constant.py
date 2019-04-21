import random
import os

if __name__ == "__main__":
    # I read on the internet that random seeds itself from urandom
    move = random.choice(["R", "P", "S"])
    while True:
        print(move)