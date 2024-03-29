
VERSION = 0.4

SRC = src/plugin.cpp src/plugin.h \
      src/cache.cpp src/cache.h \
      src/filecache.cpp src/filecache.h \
      src/netcache.cpp src/netcache.h \
      src/webdav-client.cpp src/webdav-client.h \
      src/tracker.h
LIB = FBuild-NetCache$(LIB_SUFFIX)
PACKAGE = fastbuild-netcache-$(VERSION)_$(PLATFORM)-x64$(PKG_SUFFIX)

ifeq ($(OS),Windows_NT)
PLATFORM = windows
LIB_SUFFIX = .dll
PKG_SUFFIX = .zip
PKG_EXTRA = $(LIB:%.dll=%.pdb)
ARCHIVE = zip
else
PLATFORM = linux
LIB_SUFFIX = .so
PKG_SUFFIX = .tar.gz
ARCHIVE = tar czf
endif

CPPFLAGS = -Wall -Wextra -DVERSION=\"$(VERSION)\"
CXXFLAGS = -std=c++20 -Os
INCLUDES = -I3rdparty/fastbuild/Code/Tools/FBuild/FBuildCore/Cache \
           -I3rdparty/cpp-httplib
LIBS = -lssl -lcrypto
ifneq ($(DEBUG),1)
LDFLAGS += -s
endif

ifeq ($(OS),Windows_NT)
ifeq ($(DEBUG),1)
CXXFLAGS += -g -gcodeview
endif
LIBS += -lws2_32 -lcrypt32
LDFLAGS += -static
else
ifeq ($(DEBUG),1)
CXXFLAGS += -g -ggdb
endif
CXXFLAGS += -fPIC
endif

OBJ = $(patsubst %.cpp, %.o, $(filter %.cpp, $(SRC)))

all: $(LIB)

dist: $(PACKAGE)

$(PACKAGE): $(LIB)
	$(ARCHIVE) $@ $^ $(PKG_EXTRA)

$(LIB): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS) -shared

%.o: $(filter %.h, $(SRC))
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(CPPFLAGS) $(INCLUDES)

clean:
	-rm -f $(OBJ) $(LIB) $(PKG_EXTRA) $(PACKAGE)

# TODO: this may be used if we don’t want to depend on mingw64
#   winget install FireDaemon.OpenSSL
#   LDFLAGS += -L"/c/Program Files/FireDaemon OpenSSL 3/lib"
