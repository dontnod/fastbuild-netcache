
# this is really a proof of concept for now
VERSION = 0.1

SRC = src/plugin.cpp src/plugin.h \
      src/netcache.cpp src/netcache.h \
      src/webdav-client.cpp src/webdav-client.h
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

ifeq ($(OS),Windows_NT)
CXXFLAGS += -g -gcodeview
LIBS += -lws2_32 -lcrypt32
LDFLAGS += -static
else
CXXFLAGS += -g -ggdb
CXXFLAGS += -fPIC
endif

OBJ = $(patsubst %.cpp, %.o, $(filter %.cpp, $(SRC)))

all: $(PACKAGE)

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
