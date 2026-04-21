include $(TOPDIR)/rules.mk

PKG_NAME:=smsforward
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE
PKG_MAINTAINER:=SMS Forward <xuehui1994@outlook.com>

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/smsforward
  SECTION:=net
  CATEGORY:=Network
  TITLE:=SMS Forward Service for OpenWRT
  DEPENDS:=
  URL:=https://github.com/example/smsforward
endef

define Package/smsforward/description
  A C implementation of SMS forwarding service for OpenWRT.
  Supports multiple push notification services:
  - PushPlus
  - WxPusher
  - PushDeer
  - WxWork Webhook
  - Bark
  - DingTalk
  - Feishu
  - iyuu
  Features:
  - JSON configuration support
  - File locking mechanism
  - SMS data validation
  - HTTP push notifications
  - Persistent data storage
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./smsforward.c $(PKG_BUILD_DIR)/
	$(CP) ./smsforward.h $(PKG_BUILD_DIR)/
	$(CP) ./json.c $(PKG_BUILD_DIR)/
	$(CP) ./json.h $(PKG_BUILD_DIR)/
	$(CP) ./serial.c $(PKG_BUILD_DIR)/
	$(CP) ./serial.h $(PKG_BUILD_DIR)/
endef

define Build/Configure
	# No configuration needed
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -Wall -Wextra -std=c99 -O2 \
		-D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
		-I$(PKG_BUILD_DIR) \
		-o $(PKG_BUILD_DIR)/smsforward \
		$(PKG_BUILD_DIR)/smsforward.c \
		$(PKG_BUILD_DIR)/json.c \
		$(PKG_BUILD_DIR)/serial.c \
		$(TARGET_LDFLAGS)
endef

define Package/smsforward/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/smsforward $(1)/usr/bin/smsforward
endef

$(eval $(call BuildPackage,smsforward))
