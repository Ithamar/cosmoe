#!/bin/sh

echo "****************************************************"
echo "** Configuring Cosmoe for distributed compilation **"
echo "****************************************************"
echo ""
CC=distcc CXX=distcc ./configure $*
