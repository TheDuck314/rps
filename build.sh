#!/bin/sh

set -ex

g++ -Wall -Werror -std=c++14 -c networking/Connection.cpp
g++ -Wall -Werror -std=c++14 -o rps main.cpp *.o

