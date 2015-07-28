#!/bin/sh
tar -zcvf npmjx305.tar.gz npm
cp npmjx305.tar.gz ~/.jx/
cd ~/.jx/
rm -rf npm
tar -xvf npmjx305.tar.gz
