## i686 and x86_64
jxcore can be installed from aur with yaourt:
'''bash
yaourt -S jxcore
'''
or using the files in either the aur or arm directories to make a tarball for pacman.
'''bash
mkdir -p ~/abs/jxcore 
cp {PKGBUILD,jxcore.install} ~/abs/jxcore
cd ~/abs/jxcore
makepkg -i
'''
