
# FASTBuild NetCache

This project allows to use an HTTP or WebDAV server as a backend for the
[FASTBuild](https://github.com/fastbuild/fastbuild) compilation cache.

This project uses [cpp-httplib](https://github.com/yhirose/cpp-httplib) as its HTTP client
library.

## Installation

Just copy the plugin (`FBuild-NetCache.dll` or `FBuild-NetCache.so`) on each machine that
will run FASTBuild. It is easier, but not required, to store it side-by-side with `FBuild.exe`.

## Usage

1. Add a reference to the plugin in the `Settings` section in the `.bff` file. If installed
in a non-standard location, the full path to the DLL may have to be specified:

```
.CachePluginDLL = 'FBuild-NetCache.dll' ; (or 'FBuild-NetCache.so' on Linux)
```

2. Ensure the `.CachePath` settings entry or the `FASTBUILD_CACHE_PATH` environment variable
refer to an HTTP URL or a WebDAV path, for instance:

 - `\\server.example.com\fbcache`
 - `\\other.server.example.com@8080\fbcache`
 - `\\secure-server.example.com@ssl\DavWWWRoot\fbcache`
 - `http://server.example.com:9000/path/to/fbuild/cache`
 - `https://secure-server.example.com/cacheroot/`

## Access control

If the HTTP or WebDAV server requires authentication, credentials can be provided in two ways:

 - stored as environment variables, for instance:
   - `FASTBUILD_CACHE_USERNAME=john`
   - `FASTBUILD_CACHE_PASSWORD=s3cr3tp4ss`
 - stored as generic credentials in the Windows Credential Manager, for instance:
   - `cmdkey /generic:server.example.com /user:john /pass:s3cr3tp4ss`

## Build instructions

Just run `make`. In order to build with another compiler, use *e.g.* `make CXX=g++-13`.

For now the best way to build on Windows is using an [MSYS2](https://www.msys2.org/) shell.

Linux package prerequisites:
 - `make`, `libssl-dev`

Windows package prerequisites:
 - `make`, `zip`
 - a C++ compiler, *e.g.* `mingw-w64-x86_64-gcc` or `mingw-w64-x86_64-clang`
 - the OpenSSL library, *e.g.* `mingw-w64-x86_64-openssl` or `mingw-w64-clang-x86_64-openssl`
