#!/bin/sh
scons client=1 debug=1 > /dev/null
scons server=1 debug=1 > /dev/null
scons botlib=1 debug=1 > /dev/null
scons rd-vanilla=1 debug=1 > /dev/null
scons rd-rust=1 debug=1 > /dev/null
scons game=1 debug=1 > /dev/null
scons cgame=1 debug=1 > /dev/null
scons ui=1 debug=1 > /dev/null
