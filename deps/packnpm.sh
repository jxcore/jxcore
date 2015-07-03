#!/bin/sh
tar -zcvf npmjx304.tar.gz npm
cp npmjx304.tar.gz ~/.jx/
cd ~/.jx/
rm -rf npm
tar -xvf npmjx304.tar.gz
