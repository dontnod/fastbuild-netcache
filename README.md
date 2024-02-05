
# FASTBuild NetCache

This project allows to use an HTTP or WebDAV server as a backend for the
[FASTBuild](https://github.com/fastbuild/fastbuild) compilation cache.

Features:

 - Windows WebDAV syntax for cache paths
 - HTTP syntax for cache paths
 - support standard file-based caches as a fallback
 - multiple cache locations
 - support for Windows Credential Manager

### Cache paths

It supports cache paths (through the `.CachePath` setting entry, or through the
`FASTBUILD_CACHE_PATH` environment variable) in a Windows-like format:

```
.CachePath = '\\server.example.com\fbcache'
.CachePath = '\\other.server.example.com@8080\fbcache'
.CachePath = '\\secure-server.example.com@ssl\DavWWWRoot\fbcache'
```

Alternatively, plain HTTP URLs are also supported:

```
.CachePath = 'http://server.example.com:9000/path/to/fbuild/cache'
.CachePath = 'https://secure-server.example.com/cacheroot/'
```

If the cache paths does not point to a network location, the plugin falls
back to a standard filesystem cache location:

```
.CachePath = 'C:\Temporary\Cache'
```

### Multiple cache locations

Multiple cache locations can be specified, separated by a semicolon. When retrieving, each cache
is queried until data is found. When sending, each cache is queried until one publishing attempt
is successful.

```
.CachePath = 'C:\Temporary\Cache;https://secure-server.example.com/cacheroot/'
```

### Credentials

If the HTTP or WebDAV server requires authentication, credentials can be provided in two ways:

 - stored as environment variables:
   - `FASTBUILD_CACHE_USERNAME=john`
   - `FASTBUILD_CACHE_PASSWORD=s3cr3tp4ss`
 - stored as generic credentials in the Windows Credential Manager:
   - `cmdkey /generic:server.example.com /user:john /pass:s3cr3tp4ss`

## Setup

1. Copy the plugin (`FBuild-NetCache.dll` or `FBuild-NetCache.so`) on each machine that
will run FASTBuild. It is easier, but not required, to store it side-by-side with `FBuild.exe`.

2. Add a reference to the plugin in the `Settings` section in the `.bff` file. If installed
in a non-standard location, the full path to the DLL may have to be specified:

```
.CachePluginDLL = 'FBuild-NetCache.dll' ; (or 'FBuild-NetCache.so' on Linux)
```

```
.CachePluginDLL = 'C:\ProgramData\FASTBuild\FBuild-NetCache.dll'
```

## Build instructions

Just run `make`. In order to build with another compiler, use *e.g.* `make CXX=g++-13`.

For debug builds, use `make DEBUG=1`.

For now the best way to build on Windows is using an [MSYS2](https://www.msys2.org/) shell.

Linux package prerequisites:
 - `make`, `libssl-dev`

Windows package prerequisites:
 - `make`, `zip`
 - a C++ compiler, *e.g.* `mingw-w64-x86_64-gcc` or `mingw-w64-x86_64-clang`
 - the OpenSSL library, *e.g.* `mingw-w64-x86_64-openssl` or `mingw-w64-clang-x86_64-openssl`

## Acknowledgements

FASTBuild NetCache development is funded by [Don’t Nod Entertainment](https://dont-nod.com/en/).

FASTBuild NetCache uses [cpp-httplib](https://github.com/yhirose/cpp-httplib) as its HTTP client
library.
