.PHONY: all clean run stop debug

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

POSIX_SRCS=$(wildcard *.cpp)
COMMON_SRCS=$(wildcard ../*.cpp)

POSIX_OBJS=$(POSIX_SRCS:%.cpp=out/%.o)
COMMON_OBJS=$(COMMON_SRCS:../%.cpp=out/%.o)

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
$(info $(LIBS))
ifeq ($(UNAME_S),Darwin)
	GDB ?= lldb
endif
GDB ?= gdb

OUT ?= out/cypherpunk-privacy-service

all: $(OUT)

-include $(POSIX_OBJS:%.o=%.d) $(COMMON_OBJS:%.o=%.d)

$(POSIX_OBJS): out/%.o : %.cpp
	@echo "* Compiling $*.cpp..."
	@mkdir -p out
	@g++ -c $(CFLAGS) -MMD -MF out/$*.d $(INCS) -o $@ $<

$(COMMON_OBJS): out/%.o : ../%.cpp
	@echo "* Compiling $*.cpp..."
	@mkdir -p out
	@g++ -c $(CFLAGS) -MMD -MF out/$*.d $(INCS) -o $@ $<

$(OUT): $(POSIX_OBJS) $(COMMON_OBJS)
	@echo "* Linking $@..."
	@mkdir -p out
	@g++ $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	@rm -rf out

stop:
	@sudo launchctl unload /Library/LaunchDaemons/com.cypherpunk.privacy.service.plist || true

run:
	@sudo $(OUT)

debug:
	@sudo $(GDB) $(OUT)
