[![Build](https://github.com/ad-freiburg/staty/actions/workflows/build.yml/badge.svg)](https://github.com/ad-freiburg/staty/actions/workflows/build.yml)
# staty

Frontend for results for OSM fixing mode of statsimi.

## Installation

    mkdir build && cd build
    cmake ..
    make
    sudo make install

After that, you can start staty like this:

    staty <path to .fix file from statsimi>

and visit http://localhost:9090
