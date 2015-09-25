#!/bin/sh
tar -zcvf npmjx307.tar.gz npm
cp npmjx307.tar.gz ~/.jx/
cd ~/.jx/
rm -rf npm
tar -xvf npmjx307.tar.gz
