#!/bin/sh
VER=311
tar -zcvf npmjx$VER.tar.gz npm
cp npmjx$VER.tar.gz ~/.jx/
cd ~/.jx/
rm -rf npm
tar -xvf npmjx$VER.tar.gz
