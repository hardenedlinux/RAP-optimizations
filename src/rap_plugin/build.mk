
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

LDFLAGS = -shared
CFLAGS = -g3 -O0 -std=gnu++98 -ggdb -fvisibility=hidden -fno-rtti -fno-exceptions -fPIC -c
#CFLAGS = -O2 -std=gnu++98 -ggdb -fvisibility=hidden -fno-rtti -fno-exceptions -fPIC -c
HEADER = -I`$(CC) -print-file-name=plugin`/include -I`$(CC) -print-file-name=plugin`/include/c-family -I..
#HEADER = -I/usr/lib/gcc/x86_64-linux-gnu/4.8/plugin/include -I../../gcc-plugins
RAP = rap_plugin.so # target lib

$(RAP): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(OBJS): %.o : %.c
	$(CXX) $(HEADER) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJS) $(RAP)
.PHONY: test_asm
test_asm:
	$(CC) -fplugin=./rap_plugin.so -fplugin-arg-rap_plugin-typecheck=call -S ./testcase/main.c
.PHONY: test_bin
test_bin:
	$(CC) -fplugin=./rap_plugin.so -fplugin-arg-rap_plugin-typecheck=call ./testcase/main.c

