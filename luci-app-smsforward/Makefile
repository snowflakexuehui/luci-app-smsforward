# Copyright (C) 2016 Openwrt.org
#
# This is free software, licensed under the Apache License, Version 2.0.
#

include $(TOPDIR)/rules.mk

LUCI_TITLE:=LuCI Support for SMS Forward
LUCI_DEPENDS:=+luci +smsforward
LUCI_PKGARCH:=all

PKG_NAME:=luci-app-smsforward
PKG_VERSION:=1.1
PKG_RELEASE:=1
PKG_MAINTAINER:=Snowflakes <xuehui1994@outlook.com>

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature
