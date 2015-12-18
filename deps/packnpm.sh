#!/bin/sh
tar -zcvf npmjx310.tar.gz npm
cp npmjx310.tar.gz ~/.jx/
cd ~/.jx/
rm -rf npm
tar -xvf npmjx310.tar.gz
