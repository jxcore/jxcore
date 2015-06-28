##############################################
# JXcore OpenWrt Makefile 
#
# This Makefile is prepared for OpenWRT package builder
##############################################

include $(TOPDIR)/rules.mk

PKG_NAME:=jxcore
PKG_RELEASE:=0.3.0.3
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/jxcore
	SECTION:=utils
	DEPENDS:=+libpthread +librt +libstdcpp
	CATEGORY:=Utilities
	TITLE:=jxcore -- node.js on mobile and IOT
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -r ./jxcore/* $(PKG_BUILD_DIR)/
endef

# By default no embedded sqlite or leveldown
# remove --no-sqlite for sqlite3 embedded
# add --embed-leveldown for embedded leveldown native 
define Build/Configure
	cd $(PKG_BUILD_DIR) ; ./configure --prefix=out_mips_wrt --dest-cpu=mipsel --no-sqlite --engine-mozilla --dest-os=openwrt
endef

define Package/jxcore/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/jx $(1)/bin/
endef

$(eval $(call BuildPackage,jxcore,+libpthread,+librt))
