# Redis模块编译规则

# 检测系统
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

# 设置编译器和标志
CC=gcc
CFLAGS=-Wall -Wextra -O2 -fPIC -std=c99
LDFLAGS=-shared
LIBS=-luuid -lssl -lcrypto

# 系统适配
ifeq ($(uname_S),Linux)
	SHOBJ_CFLAGS ?= -W -Wall -fno-common -g -ggdb -std=c99 -O2
	SHOBJ_LDFLAGS ?= -shared
else
ifeq ($(uname_S),Darwin)
	SHOBJ_CFLAGS ?= -W -Wall -dynamic -fno-common -g -ggdb -std=c99 -O2
	SHOBJ_LDFLAGS ?= -bundle -undefined dynamic_lookup
else
	SHOBJ_CFLAGS ?= -W -Wall -fno-common -g -ggdb -std=c99 -O2
	SHOBJ_LDFLAGS ?= -shared
endif
endif

# 源文件和目标
SOURCES=powmodule.c
MODULE_NAME=powmodule
MODULE_SO=$(MODULE_NAME).so

# 包含Redis模块头文件
INCLUDE=-I../deps/redis/src

# 默认目标
all: $(MODULE_SO)

# 下载Redis源码以获取redismodule.h
../deps/redis:
	mkdir -p ../deps
	git clone --depth 1 https://github.com/redis/redis.git ../deps/redis

# 复制redismodule.h到当前目录
redismodule.h: ../deps/redis
	cp ../deps/redis/src/redismodule.h ./

# 编译模块
$(MODULE_SO): redismodule.h $(SOURCES)
	$(CC) $(CFLAGS) $(SHOBJ_CFLAGS) $(INCLUDE) -o $@ $(SOURCES) $(LDFLAGS) $(SHOBJ_LDFLAGS) $(LIBS)

# 测试
test: $(MODULE_SO)
	redis-server --loadmodule ./$(MODULE_SO) --port 6379 --daemonize no --bind 0.0.0.0 --protected-mode no

# 安装模块
install: $(MODULE_SO)
	cp $(MODULE_SO) /usr/local/lib/

# 清理
clean:
	rm -f *.o *.so *.xo *.a

.PHONY: all install clean
