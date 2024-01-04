
SRC = fbuild-netcache.cpp
DLL = fbuild-netcache.dll

CPPFLAGS = -Wall
INCLUDES = -I3rdparty
CXXFLAGS = -std=c++20 -Os
LDFLAGS = -shared
LIBS = -lws2_32 -lcrypt32 -lssl -lcrypto

$(DLL): $(SRC)
	$(CXX) $(CPPFLAGS) $(INCLUDES) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

clean:
	-rm -f $(DLL)

# TODO: this may be used if we don’t want to depend on mingw64
#   winget install FireDaemon.OpenSSL
#   LDFLAGS += -L"/c/Program Files/FireDaemon OpenSSL 3/lib"
