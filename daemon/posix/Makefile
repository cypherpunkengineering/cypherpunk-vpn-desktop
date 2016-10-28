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
	-o cypherpunkvpn-service

stop:
	sudo launchctl unload /Library/LaunchDaemons/com.cypherpunk.vpn.service.plist || true

run:
	sudo ./cypherpunkvpn-service

