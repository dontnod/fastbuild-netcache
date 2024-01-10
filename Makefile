
# this is really a proof of concept for now
VERSION = 0.0.2

SRC = netcache.cpp webdav-client.h
DLL = FBuild-NetCache.dll
ZIP = fastbuild-netcache-$(VERSION)_windows-x64.zip

CPPFLAGS = -Wall -DVERSION=\"$(VERSION)\"
INCLUDES = -I3rdparty
CXXFLAGS = -std=c++20 -Os
LDFLAGS = -static
LIBS = -lssl -lcrypto -lws2_32 -lcrypt32

OBJ = $(patsubst %.cpp, %.o, $(filter %.cpp, $(SRC)))

all: $(ZIP)

$(ZIP): $(DLL)
	zip $@ $^

$(DLL): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS) -shared

%.o: %.cpp $(filter %.h, $(SRC))
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(CPPFLAGS) $(INCLUDES)

clean:
	-rm -f $(OBJ) $(DLL) $(ZIP)

# TODO: this may be used if we don’t want to depend on mingw64
#   winget install FireDaemon.OpenSSL
#   LDFLAGS += -L"/c/Program Files/FireDaemon OpenSSL 3/lib"
