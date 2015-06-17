#!/bin/sh
tar -zcvf npmjx303.tar.gz npm
cp npmjx303.tar.gz ~/.jx/
cd ~/.jx/
rm -rf npm
tar -xvf npmjx303.tar.gz
