#!/bin/sh

set -ex

if [ -d tclap-1.2.2 ] ; then
    echo 'tclap-1.2.2 already present, not downloading'
else
    echo 'Downloading the tclap command line parsing library...'
    mkdir -p downloads
    wget -P ./downloads https://sourceforge.net/projects/tclap/files/tclap-1.2.2.tar.gz
    # check that we got the expected file:
    echo 'Checking tclap checksum...'
    echo 'f5013be7fcaafc69ba0ce2d1710f693f61e9c336b6292ae4f57554f59fde5837 *downloads/tclap-1.2.2.tar.gz' | shasum -c -a 256
    tar xzvf downloads/tclap-1.2.2.tar.gz
fi

g++ -Wall -Werror -std=c++14 -c networking/Connection.cpp
g++ -Wall -Werror -std=c++14 -I./tclap-1.2.2/include/ -o rps main.cpp *.o

