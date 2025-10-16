CXX       ?= g++
CXXSTD    ?= -std=c++20

PKG_CFLAGS := $(shell pkg-config --cflags libmariadb 2>/dev/null) \
              $(shell pkg-config --cflags openssl 2>/dev/null)
PKG_LIBS   := $(shell pkg-config --libs   libmariadb 2>/dev/null) \
              $(shell pkg-config --libs   openssl    2>/dev/null)

FALLBACK_INC := -I/usr/include/mariadb -I/usr/include/mysql
MYSQL_LIB := $(shell pkg-config --exists libmariadb && echo -lmariadb || echo -lmysqlclient)
FALLBACK_LIBS := $(MYSQL_LIB) -lssl -lcrypto

INC  := $(if $(strip $(PKG_CFLAGS)),$(PKG_CFLAGS),$(FALLBACK_INC))
LIBS := $(if $(strip $(PKG_LIBS)),$(PKG_LIBS),$(FALLBACK_LIBS))

SRC  := $(wildcard *.cpp)
CGIS := $(patsubst %.cpp,%.cgi,$(SRC))

.PHONY: all clean
all: $(CGIS)

%.cgi: %.cpp
	$(CXX) $(CXXSTD) $(INC) $< -o $@ $(LIBS)
	chmod 755 $@

clean:
	rm -f *.cgi
