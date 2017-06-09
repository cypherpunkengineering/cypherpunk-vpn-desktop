.PHONY: Makefile all clean version vpn great again \
	package client daemon update-node-modules check-node-modules \
	stage stage-client stage-daemon \
	install install-client install-daemon \
	start-daemon stop-daemon \
	uninstall uninstall-client uninstall-daemon
.DEFAULT: all
.SUFFIXES:

HUSH := @
ECHO := @echo
QUIET := > /dev/null
exec = $(HUSH) { $@ ; } $(QUIET)

ROOT = $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)
OUT = $(ROOT)/out/osx
STAGING = $(OUT)/staging

APP_NAME = Cypherpunk Privacy
APP_ARCH := x64
APP_PLATFORM := darwin
APP_SIG = $(APP_NAME)-$(APP_PLATFORM)-$(APP_ARCH)
APP_BUNDLE = $(APP_NAME).app
APP_NS = com.cypherpunk.privacy
CLIENT_NS = $(APP_NS).client
DAEMON_NS = $(APP_NS).service
DAEMON_FILENAME = cypherpunk-privacy-service
OPENVPN_FILENAME = cypherpunk-privacy-openvpn
OPENVPN_SCRIPTS = $(shell find $(ROOT)/res/osx/openvpn-scripts -type f -name '*.sh')
PF_ANCHORS = $(shell find $(ROOT)/res/osx/pf.anchors -type f -name 'com.cypherpunk.*')

APP_SIGN_IDENTITY ?= "Developer ID Application: Cypherpunk Engineering K.K. (S353YJSBDX)"
INSTALLER_SIGN_IDENTITY ?= "Developer ID Installer: Cypherpunk Engineering K.K. (S353YJSBDX)"


ifneq ($(MAKECMDGOALS),clean)

ifeq ($(wildcard $(ROOT)/client/node_modules/.),)
$(info * Initializing node modules...)
ifneq ($(shell cd "$(ROOT)/client" && npm install 1>&2 || echo FAIL),)
$(error Failed to initialize node modules!)
endif
endif
$(info * Generating version number...)
$(info $(shell rm -f "$(ROOT)/version.txt" && cd "$(ROOT)/client" && npm run build-version 1>&2 && npm run apply-version 1>&2))
export APP_VER := $(shell cat "$(ROOT)/version.txt")
ifeq ($(APP_VER),)
$(error Failed to generate version number!)
endif
export APP_SHORT_VER := $(shell echo "$(APP_VER)" | cut -d. -f1-2)
define COPYRIGHT_STRING
™ and © $(shell date +%Y) Cypherpunk Partners, slf.
All rights reserved.
endef
export COPYRIGHT_STRING

endif

INSTALLER_PKG = cypherpunk-privacy-macos-$(subst +,-,$(APP_VER)).pkg


all: package

package: $(OUT)/$(INSTALLER_PKG)

clean:
	$(ECHO) "* Cleaning..."
	$(HUSH) rm -rf "$(OUT)"

vpn: clean
great: update-node-modules
again: package

define do-stop-client
	$(HUSH) osascript -e 'quit app "Cypherpunk Privacy"' && sleep 2 || true $(QUIET)
endef
define do-start-daemon
	$(HUSH) sudo launchctl load "/Library/LaunchDaemons/$(DAEMON_NS).plist"
endef
define do-stop-daemon
	$(HUSH) sudo launchctl unload "/Library/LaunchDaemons/$(DAEMON_NS).plist" || true
endef

start-daemon:
	$(do-start-daemon)

stop-daemon:
	$(do-stop-daemon)


# Source building

client:
	$(ECHO) "* Building client..."
	$(HUSH) cd "$(ROOT)/client" && npm --production run build
	$(ECHO) "* Rebuilding Electron modules..."
	$(HUSH) cd "$(ROOT)/client" && ./node_modules/.bin/electron-rebuild

daemon: $(ROOT)/daemon/posix/out/$(DAEMON_FILENAME)

$(ROOT)/daemon/posix/out/$(DAEMON_FILENAME): version
	$(HUSH) $(MAKE) -C $(ROOT)/daemon/posix RELEASE=1 all


# Staging

ECHO_STAGING=$(ECHO) "* Staging $(@:$(STAGING)%=%)..."

stage: stage-client stage-daemon

stage-client: client
	$(ECHO) "* Staging client..."
	$(HUSH) rm -rf "$(OUT)/$(APP_SIG)"
	$(HUSH) cd "$(ROOT)/client" && ./node_modules/.bin/electron-packager ./app/ "$(APP_NAME)" \
		--app-bundle-id="$(APP_NS).app" \
		--app-version="Version $(APP_SHORT_VER)" \
		--build-version="v$(APP_VER)" \
		--app-copyright="$$COPYRIGHT_STRING" \
		--platform=darwin \
		--arch=x64 \
		--icon="$(ROOT)/res/osx/logo5.icns" \
		--out="$(OUT)"
	$(HUSH) sync; sleep 1
	$(HUSH) codesign --force --deep --sign $(APP_SIGN_IDENTITY) "$(OUT)/$(APP_SIG)/$(APP_BUNDLE)"
	$(HUSH) rm -rf "$(STAGING)/Applications/$(APP_BUNDLE)"
	$(HUSH) mkdir -p $(STAGING)/Applications
	$(HUSH) mv "$(OUT)/$(APP_SIG)/$(APP_BUNDLE)" "$(STAGING)/Applications/"
	$(HUSH) rm -rf "$(OUT)/$(APP_SIG)"

stage-daemon: $(STAGING)/usr/local/cypherpunk/bin/$(DAEMON_FILENAME) $(STAGING)/Library/LaunchDaemons/$(DAEMON_NS).plist $(STAGING)/usr/local/cypherpunk/bin/$(OPENVPN_FILENAME) $(OPENVPN_SCRIPTS:$(ROOT)/res/osx/openvpn-scripts/%=$(STAGING)/usr/local/cypherpunk/etc/scripts/%) $(PF_ANCHORS:$(ROOT)/res/osx/pf.anchors/%=$(STAGING)/usr/local/cypherpunk/etc/pf.anchors/%)

$(STAGING)/usr/local/cypherpunk/bin/$(DAEMON_FILENAME): $(ROOT)/daemon/posix/out/$(DAEMON_FILENAME)
	$(ECHO_STAGING)
	$(HUSH) mkdir -p $(dir $@)
	$(HUSH) install -c -m 755 $< $@
	$(HUSH) codesign --sign $(APP_SIGN_IDENTITY) $@
$(STAGING)/usr/local/cypherpunk/bin/$(OPENVPN_FILENAME): $(ROOT)/daemon/third_party/openvpn_osx/openvpn
	$(ECHO_STAGING)
	$(HUSH) mkdir -p $(dir $@)
	$(HUSH) install -c -m 755 $< $@
	$(HUSH) codesign --force --sign $(APP_SIGN_IDENTITY) $@
$(STAGING)/usr/local/cypherpunk/etc/pf.anchors/%: $(ROOT)/res/osx/pf.anchors/%
	$(ECHO_STAGING)
	$(HUSH) mkdir -p $(dir $@)
	$(HUSH) cp -p $< $@
$(STAGING)/usr/local/cypherpunk/etc/scripts/%: $(ROOT)/res/osx/openvpn-scripts/%
	$(ECHO_STAGING)
	$(HUSH) mkdir -p $(dir $@)
	$(HUSH) install -c -m 755 $< $@
$(STAGING)/Library/LaunchDaemons/$(DAEMON_NS).plist: $(ROOT)/res/osx/plist/$(DAEMON_NS).plist
	$(ECHO_STAGING)
	$(HUSH) mkdir -p $(dir $@)
	$(HUSH) cp -p $< $@


# Installing to local machine

install: install-daemon install-client

install-client: stage-client uninstall-client
	$(HUSH) sudo cp -pR "$(STAGING)/Applications/$(APP_BUNDLE)" /Applications/
	$(HUSH) sudo chown -R root:admin "/Applications/$(APP_BUNDLE)"
	$(HUSH) sudo chgrp cypherpunk "/Applications/$(APP_BUNDLE)/Contents/MacOS/$(APP_NAME)"
	$(HUSH) sudo chmod g+s "/Applications/$(APP_BUNDLE)/Contents/MacOS/$(APP_NAME)"

install-daemon: stage-daemon
	$(HUSH) sudo chown -R root:admin $(STAGING)/Library
	$(HUSH) sudo chown -R root:wheel $(STAGING)/usr
	$(do-stop-daemon)
	$(HUSH) sudo mkdir -p /usr/local/cypherpunk
	$(HUSH) sudo cp -f "$(STAGING)/Library/LaunchDaemons/$(DAEMON_NS).plist" /Library/LaunchDaemons/
	$(HUSH) sudo rm -f "/usr/local/cypherpunk/bin/$(DAEMON_FILENAME)" "/usr/local/cypherpunk/bin/$(OPENVPN_FILENAME)"
	$(HUSH) sudo cp -R "$(STAGING)/usr/local/cypherpunk" /usr/local/
	$(HUSH) sudo dscl . -create /Groups/cypherpunk gid 333
	$(HUSH) sudo dseditgroup -o edit -a everyone -t group cypherpunk
	$(do-start-daemon)


# Uninstalling from local machine

uninstall: uninstall-client uninstall-daemon

uninstall-client:
	$(do-stop-client)
	$(HUSH) sudo rm -rf "/Applications/$(APP_BUNDLE)"
#TODO: Kill app

uninstall-daemon:
	$(do-stop-daemon)
	$(HUSH) sudo rm -f "/Library/LaunchDaemons/$(DAEMON_NS).plist"


# Final packages

$(OUT)/Build.pkg: stage
	$(HUSH) chmod +x "$(ROOT)/res/osx/install-scripts/preinstall"
	$(HUSH) chmod +x "$(ROOT)/res/osx/install-scripts/postinstall"
	$(HUSH) cd "$(OUT)" && pkgbuild \
		--root "$(STAGING)" \
		--scripts "$(ROOT)/res/osx/install-scripts" \
		--sign $(INSTALLER_SIGN_IDENTITY) \
		--identifier "${APP_NS}.pkg" \
		--version "${APP_VER}" \
		--ownership recommended \
		--install-location / \
		Build.pkg

$(OUT)/$(INSTALLER_PKG): $(OUT)/Build.pkg
	cd "$(OUT)" && productbuild \
		--resources "$(ROOT)/res/osx/resources" \
		--distribution "$(ROOT)/res/osx/resources/distribution.xml" \
		--sign $(INSTALLER_SIGN_IDENTITY) \
		--version "$(APP_VER)" \
		"$(INSTALLER_PKG)"
	$(ECHO) "* Finished: $$(ls -l $(OUT)/$(INSTALLER_PKG))"
