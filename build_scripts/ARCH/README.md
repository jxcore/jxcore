## i686 and x86_64
jxcore can be installed from aur with yaourt:
'''bash
yaourt -S jxcore
'''
or using this PKGBUILD file to make a tarball that can be installed with pacman.
For i686 or x86_64 systems 
'''bash
mkdir -p ~/abs/jxcore 
cp PKGBUILD ~/abs/jxcore
cd ~/abs/jxcore
makepkg -s
sudo pacman -U jxcore-VERSION-arch.tar.xz
'''