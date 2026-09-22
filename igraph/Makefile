CC ?= gcc
CFLAGS ?= -O3 -Wall -Wextra

# 1. Detectar si pkg-config tiene configurado igraph
PKG_CFLAGS := $(shell pkg-config --cflags igraph 2>/dev/null)
PKG_LIBS   := $(shell pkg-config --libs igraph 2>/dev/null)

# 2. Detectar si existe una copia compilada localmente en ~/igraph
LOCAL_IGRAPH := $(HOME)/igraph
LOCAL_LIB    := $(LOCAL_IGRAPH)/build/src/libigraph.a

ifneq ($(PKG_LIBS),)
    CFLAGS  += $(PKG_CFLAGS)
    LDFLAGS += $(PKG_LIBS) -lm
else ifneq ($(wildcard $(LOCAL_LIB)),)
    CFLAGS  += -I$(LOCAL_IGRAPH)/include -I$(LOCAL_IGRAPH)/build/include
    LDFLAGS += $(LOCAL_LIB) -lm -lstdc++ -lgomp -lpthread
else
    # Por defecto: usar igraph instalado a nivel de sistema (ej: sudo apt install libigraph-dev)
    LDFLAGS += -ligraph -lm
endif

TARGET = benchmark_c
SRC = benchmark_c.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
