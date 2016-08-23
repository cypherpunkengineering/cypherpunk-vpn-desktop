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
	$(INCS) \
	$(SRCS) \
	-o cyphervpn-service
