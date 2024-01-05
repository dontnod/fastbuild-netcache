
# this is really a proof of concept for now
VERSION = 0.0.1

SRC = fbuild-netcache.cpp
DLL = fbuild-netcache.dll
ZIP = fbuild-netcache-$(VERSION)_windows-x64.zip

CPPFLAGS = -Wall -DVERSION=\"$(VERSION)\"
INCLUDES = -I3rdparty
CXXFLAGS = -std=c++20 -Os
LDFLAGS = -shared -static
LIBS = -lssl -lcrypto -lws2_32 -lcrypt32

$(ZIP): $(DLL)
	zip $@ $^

$(DLL): $(SRC)
	$(CXX) $(CPPFLAGS) $(INCLUDES) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

clean:
	-rm -f $(DLL)

# TODO: this may be used if we don’t want to depend on mingw64
#   winget install FireDaemon.OpenSSL
#   LDFLAGS += -L"/c/Program Files/FireDaemon OpenSSL 3/lib"
