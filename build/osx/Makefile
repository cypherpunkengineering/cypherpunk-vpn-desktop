.PHONY: Makefile all clean version vpn great again \
	package client daemon update-node-modules check-node-modules \
	stage stage-client stage-daemon stage-openvpn stage-extensions \
	install install-client install-daemon install-extensions \
	start-daemon stop-daemon \
	uninstall uninstall-client uninstall-daemon uninstall-extensions
.DEFAULT: all
.SUFFIXES:

#USE_EXTENSIONS := YES
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

APP_SIGN_IDENTITY = "Developer ID Application: Cypherpunk Engineering K.K. (S353YJSBDX)"
INSTALLER_SIGN_IDENTITY = "Developer ID Installer: Cypherpunk Engineering K.K. (S353YJSBDX)"

# BUILD_NUMBER is provided on Jenkins builds, or can be specified explicitly.
# If BUILD_NAME is specified (e.g. for special/branch builds), it is prepended to the build number.
ifeq ($(BUILD_NUMBER),)
BUILD_NUMBER := $(BUILD_NAME)-$(shell git --git-dir="$(ROOT)/.git" describe --always --match=nosuchtagpattern --dirty=-p)
BUILD_NUMBER := $(BUILD_NUMBER:-%=%)
endif
ORIGINAL_APP_VER := $(shell node -e "console.log(require('$(ROOT)/client/package.json').version);")
export APP_VER := $(shell echo "$(ORIGINAL_APP_VER)" | sed -E 's/^([^-+]*)(-[^+]*)?(\+.*)?/\1\2+$(BUILD_NUMBER)/')

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
define do-uninstall-extension
	$(HUSH) sudo kextunload -b net.sf.tuntaposx.$1 || true
	$(HUSH) sudo rm -rf /Library/Extensions/$1.kext
endef

start-daemon:
	$(do-start-daemon)

stop-daemon:
	$(do-stop-daemon)


# Source building

update-node-modules:
	$(ECHO) "* Updating node modules..."
	$(HUSH) cd "$(ROOT)/client" && npm install

check-node-modules: $(if $(wildcard $(ROOT)/client/node_modules/.),,update-node-modules)

#ifeq ($(APP_VER),$(ORIGINAL_APP_VER))
#version:
#	$(ECHO) "* Building version $(APP_VER)..."
#else
version: check-node-modules
	$(ECHO) "* Setting version number to $(APP_VER)..."
	$(HUSH) cd "$(ROOT)/client" && ./node_modules/.bin/json -I -f package.json -e 'this.version=this.version.replace(/(\+.*)?$$/,"+$(BUILD_NUMBER)")' && npm run version
## We'd run npm version $(APP_VER) instead but that doesn't work with build numbers
#endif

client: check-node-modules version
	$(ECHO) "* Building client..."
	$(HUSH) cd "$(ROOT)/client" && npm --production run build
	$(ECHO) "* Rebuilding Electron modules..."
	$(HUSH) cd "$(ROOT)/client" && ./node_modules/.bin/electron-rebuild

daemon: $(ROOT)/daemon/posix/$(DAEMON_FILENAME)

$(ROOT)/daemon/posix/$(DAEMON_FILENAME): version
	$(HUSH) $(MAKE) -C $(ROOT)/daemon/posix


# Staging

stage: stage-client stage-daemon stage-openvpn $(if $(USE_EXTENSIONS),stage-extensions)

stage-client: client | $(STAGING)/Applications
	$(ECHO) "* Staging client..."
	$(HUSH) rm -rf "$(OUT)/$(APP_SIG)"
	$(HUSH) cd "$(ROOT)/client" && ./node_modules/.bin/electron-packager ./app/ "$(APP_NAME)" \
		--app-bundle-id="$(APP_NS).app" \
		--platform=darwin \
		--arch=x64 \
		--icon="$(ROOT)/res/osx/logo2.icns" \
		--out="$(OUT)"
	$(HUSH) sync; sleep 1
	$(HUSH) codesign --force --deep --sign $(APP_SIGN_IDENTITY) "$(OUT)/$(APP_SIG)/$(APP_BUNDLE)"
	$(HUSH) rm -rf "$(STAGING)/Applications/$(APP_BUNDLE)"
	$(HUSH) mv "$(OUT)/$(APP_SIG)/$(APP_BUNDLE)" "$(STAGING)/Applications/"
	$(HUSH) rm -rf "$(OUT)/$(APP_SIG)"

stage-daemon: $(STAGING)/usr/local/cypherpunk/bin/$(DAEMON_FILENAME) $(STAGING)/Library/LaunchDaemons/$(DAEMON_NS).plist

stage-openvpn: $(STAGING)/usr/local/cypherpunk/bin/$(OPENVPN_FILENAME) $(OPENVPN_SCRIPTS:$(ROOT)/res/osx/openvpn-scripts/%=$(STAGING)/usr/local/cypherpunk/etc/scripts/%) $(STAGING)/Library/LaunchDaemons/$(DAEMON_NS).plist

$(addprefix $(STAGING),/Applications /usr/local/cypherpunk/bin /usr/local/cypherpunk/etc /usr/local/cypherpunk/etc/scripts /Library/Extensions /Library/LaunchAgents /Library/LaunchDaemons):
	$(HUSH) mkdir -p $@

$(STAGING)/usr/local/cypherpunk/bin/$(DAEMON_FILENAME): $(ROOT)/daemon/posix/$(DAEMON_FILENAME) | $(STAGING)/usr/local/cypherpunk/bin
	$(ECHO) "* Staging daemon..."
	$(HUSH) install -c -m 755 $< $@
	$(HUSH) codesign --sign $(APP_SIGN_IDENTITY) "$(STAGING)/usr/local/cypherpunk/bin/$(DAEMON_FILENAME)"
$(STAGING)/usr/local/cypherpunk/bin/$(OPENVPN_FILENAME): $(ROOT)/daemon/third_party/openvpn_osx/openvpn | $(STAGING)/usr/local/cypherpunk/bin
	$(ECHO) "* Staging OpenVPN binary..."
	$(HUSH) install -c -m 755 $< $@
	$(HUSH) codesign --force --sign $(APP_SIGN_IDENTITY) "$(STAGING)/usr/local/cypherpunk/bin/$(OPENVPN_FILENAME)"
$(STAGING)/usr/local/cypherpunk/etc/scripts/%: $(ROOT)/res/osx/openvpn-scripts/% | $(STAGING)/usr/local/cypherpunk/etc/scripts
	$(HUSH) cp -p $< $@
$(STAGING)/Library/LaunchDaemons/$(DAEMON_NS).plist: $(ROOT)/res/osx/plist/$(DAEMON_NS).plist | $(STAGING)/Library/LaunchDaemons
	$(HUSH) cp -p $< $@
$(STAGING)/Library/Extensions/%.kext: $(ROOT)/daemon/third_party/tuntap_osx/%.kext | $(STAGING)/Library/Extensions
	$(HUSH) cp -pR $< $@
$(patsubst %,$(STAGING)/Library/LaunchDaemons/%.plist,tap tun): $(ROOT)/daemon/third_party/tuntap_osx/%.plist | $(STAGING)/Library/LaunchDaemons
	$(HUSH) cp -p $< $@


# Installing to local machine

install: install-daemon install-client

install-client: stage-client
	$(HUSH) sudo cp -pR "$(STAGING)/Applications/$(APP_BUNDLE)" /Applications/

install-daemon: stage-daemon stage-openvpn $(if $(USE_EXTENSIONS),install-extensions)
	$(do-stop-daemon)
	$(HUSH) sudo mkdir -p /usr/local/cypherpunk
	$(HUSH) sudo cp -f "$(STAGING)/Library/LaunchDaemons/$(DAEMON_NS).plist" /Library/LaunchDaemons/
	$(HUSH) sudo rm -f "/usr/local/cypherpunk/bin/$(DAEMON_FILENAME)" "/usr/local/cypherpunk/bin/$(OPENVPN_FILENAME)"
	$(HUSH) sudo cp -R "$(STAGING)/usr/local/cypherpunk" /usr/local/
	$(do-start-daemon)

install-extensions: /Library/Extensions/tap.kext /Library/Extensions/tun.kext

/Library/Extensions/%.kext: $(STAGING)/Library/Extensions/%.kext
	$(HUSH) sudo cp -pR "$(STAGING)/Library/Extensions/$*.kext" /Library/Extensions/
	$(HUSH) sudo kextload /Library/Extensions/$*.kext


# Uninstalling from local machine

uninstall: uninstall-client uninstall-daemon

uninstall-client:
	$(do-stop-client)
	$(HUSH) sudo rm -rf "/Applications/$(APP_BUNDLE)"
#TODO: Kill app

uninstall-daemon: $(if $(USE_EXTENSIONS),uninstall-extensions)
	$(do-stop-daemon)
	$(HUSH) sudo rm -f "/Library/LaunchDaemons/$(DAEMON_NS).plist"

uninstall-extensions:
	$(ECHO) "* Uninstalling kernel extensions..."
	$(call do-uninstall-extension,tap)
	$(call do-uninstall-extension,tun)


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
