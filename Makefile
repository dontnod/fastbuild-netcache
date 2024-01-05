
# this is really a proof of concept for now
VERSION = 0.0.1

SRC = netcache.cpp
DLL = FBuild-NetCache.dll
ZIP = fastbuild-netcache-$(VERSION)_windows-x64.zip

CPPFLAGS = -Wall -DVERSION=\"$(VERSION)\"
INCLUDES = -I3rdparty
CXXFLAGS = -std=c++20 -Os
LDFLAGS = -static
LIBS = -lssl -lcrypto -lws2_32 -lcrypt32

$(ZIP): $(DLL)
	zip $@ $^

$(DLL): $(SRC)
	$(CXX) $(CPPFLAGS) $(INCLUDES) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS) -shared

clean:
	-rm -f $(DLL) $(ZIP)

# TODO: this may be used if we don’t want to depend on mingw64
#   winget install FireDaemon.OpenSSL
#   LDFLAGS += -L"/c/Program Files/FireDaemon OpenSSL 3/lib"
