.PHONY: all run stop
.DEFAULT: all

SRCS= \
	$(wildcard *.cpp) \
	$(wildcard ../*.cpp) \

INCS=\
	-I../ \
	-I../third_party/asio/asio/include \
	-I../third_party/websocketpp \
	-I../third_party/rapidjson/include/ \
	-I../third_party/jsonrpc-lean/include \

all:
	g++ \
	-std=c++14 \
	-D_DEBUG \
	$(INCS) \
	$(SRCS) \
	-o cypherpunk-privacy-service

stop:
	sudo launchctl unload /Library/LaunchDaemons/com.cypherpunk.privacy.service.plist || true

run:
	sudo ./cypherpunk-privacy-service

