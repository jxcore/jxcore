Install on archlinuxarm
using this PKGBUILD 
'''bash
mkdir -p ~/abs/jxcore 
cp PKGBUILD ~/abs/jxcore
cd ~/abs/jxcore
makepkg -s
sudo pacman -U jxcore-VERSION-arch.tar.xz
'''