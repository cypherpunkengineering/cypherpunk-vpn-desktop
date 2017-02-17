.PHONY: all clean run stop
.DEFAULT: all

DEBUG ?= true

ifneq ($(strip $(RELEASE)),)
DEBUG =
endif

CFLAGS += -std=c++14
ifneq ($(strip $(DEBUG)),)
CFLAGS += -g -D_DEBUG=1
else
CFLAGS += -O2 -D_RELEASE=1
endif

SRCS= \
	$(wildcard *.cpp) \
	$(wildcard ../*.cpp) \

INCS=\
	-I../ \
	-I../third_party/asio/asio/include \
	-I../third_party/websocketpp \
	-I../third_party/rapidjson/include/ \
	-I../third_party/jsonrpc-lean/include \

LIBS=
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LIBS += -lpthread
endif

all:
	mkdir -p out
	g++ \
	$(CFLAGS) \
	$(INCS) \
	$(SRCS) \
	$(LIBS) \
	-o out/cypherpunk-privacy-service

clean:
	rm -rf out

stop:
	sudo launchctl unload /Library/LaunchDaemons/com.cypherpunk.privacy.service.plist || true

run:
	sudo ./out/cypherpunk-privacy-service

