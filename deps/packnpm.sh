tar -zcvf npmjx.tar.gz npm
mv npmjx.tar.gz ~/.jx/
cd ~/.jx/
rm -rf npm
tar -xvf npmjx.tar.gz
