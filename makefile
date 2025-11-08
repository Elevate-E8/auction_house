CXX       ?= g++
CXXSTD    ?= -std=c++20
CXXFLAGS  := $(CXXSTD) -Wall -O2

PKG_CFLAGS := $(shell pkg-config --cflags libmariadb 2>/dev/null) \
              $(shell pkg-config --cflags openssl 2>/dev/null)
PKG_LIBS   := $(shell pkg-config --libs libmariadb 2>/dev/null) \
              $(shell pkg-config --libs openssl 2>/dev/null)

FALLBACK_INC := -I/usr/include/mariadb -I/usr/include/mysql
MYSQL_LIB := $(shell pkg-config --exists libmariadb && echo -lmariadb || echo -lmysqlclient)
FALLBACK_LIBS := $(MYSQL_LIB) -lssl -lcrypto

INC  := -Isrc $(if $(strip $(PKG_CFLAGS)),$(PKG_CFLAGS),$(FALLBACK_INC))
LIBS := $(if $(strip $(PKG_LIBS)),$(PKG_LIBS),$(FALLBACK_LIBS))


SRC_DIR := src
OUT_DIR := $(HOME)/public_html/cgi

CORE_SRCS   := $(SRC_DIR)/core/Page.cpp $(SRC_DIR)/core/Database.cpp $(SRC_DIR)/core/Session.cpp
UTILS_SRCS  := $(SRC_DIR)/utils/utils.cpp
PAGE_SRCS   := $(SRC_DIR)/pages/IndexPage.cpp \
               $(SRC_DIR)/pages/LoginPage.cpp \
               $(SRC_DIR)/pages/RegisterPage.cpp \
               $(SRC_DIR)/pages/LogoutPage.cpp \
               $(SRC_DIR)/pages/TransactionsPage.cpp \
               $(SRC_DIR)/pages/SellPage.cpp

MAIN_SRCS := $(wildcard $(SRC_DIR)/main_*.cpp)
CGIS := $(patsubst $(SRC_DIR)/main_%.cpp,%,$(MAIN_SRCS))

CSS_SRC_DIR  := css
CSS_DEST_DIR := $(HOME)/public_html/css
CSS_FILES    := $(wildcard $(CSS_SRC_DIR)/*.css)

PHONY: all
all: $(CGIS) css

$(CGIS):
	@mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) $(INC) $(SRC_DIR)/main_$@.cpp $(CORE_SRCS) $(UTILS_SRCS) $(PAGE_SRCS) -o $(OUT_DIR)/$@.cgi $(LIBS)
	chmod 755 $(OUT_DIR)/$@.cgi

.PHONY: css
css:
	@mkdir -p "$(CSS_DEST_DIR)"
	@if [ -d "$(CSS_SRC_DIR)" ] && ls $(CSS_SRC_DIR)/*.css >/dev/null 2>&1; then \
		cp -u $(CSS_SRC_DIR)/*.css "$(CSS_DEST_DIR)/"; \
	fi

.PHONY: clean clean-css
clean:
	@rm -f $(OUT_DIR)/*.cgi

clean-css:
	@rm -f $(CSS_DEST_DIR)/*.css
