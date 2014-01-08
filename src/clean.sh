#!/bin/sh
scons client=1 -c > /dev/null
scons server=1 -c > /dev/null
scons botlib=1 -c > /dev/null
scons rd-vanilla=1 -c > /dev/null
scons rd-rust=1 -c > /dev/null
scons game=1 -c > /dev/null
scons cgame=1 -c > /dev/null
scons ui=1 -c > /dev/null
