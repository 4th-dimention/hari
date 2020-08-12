#!/bin/bash

code="$PWD"
opts=-g
cd build > /dev/null
g++ $opts $code/hari_main.cpp -o hari
cd $code > /dev/null
