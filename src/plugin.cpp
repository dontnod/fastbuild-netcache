//
//  FASTBuild Network Cache Plugin
//
//  Copyright © 2024 Don’t Nod Entertainment S.A. All rights reserved.
//
//  Authors: Sam Hocevar <sam@dont-nod.com>
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
//  and associated documentation files (the "Software"), to deal in the Software without restriction,
//  including without limitation the rights to use, copy, modify, merge, publish, distribute,
//  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all copies or
//  substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#if _WIN32
#   define __WINDOWS__ 1
#elif __linux__
#   define __LINUX__ 1
#endif
#include <CachePluginInterface.h>

#include <memory> // for std::shared_ptr

#include "netcache.h"

//
// Global variable storing the cache plugin data; the current API does not allow
// to track a state or a closure, so this has to be global.
//

static std::shared_ptr<netcache> g_plugin;

//
// FASTBuild cache plugin API implementation
//

extern "C" bool CacheInitEx(const char *cachePath,
                            bool /* cacheRead */,
                            bool /* cacheWrite */,
                            bool /* cacheVerbose */,
                            const char * /* userConfig */,
                            CacheOutputFunc outputFunc)
{
    g_plugin = std::make_shared<netcache>(outputFunc);
    return g_plugin->init(cachePath);
}

extern "C" void CacheShutdown()
{
    g_plugin.reset();
}

extern "C" bool CachePublish(char const *cacheId, const void *data, size_t dataSize)
{
    return g_plugin->publish(cacheId, data, dataSize);
}

extern "C" bool CacheRetrieve(char const *cacheId, void * &data, size_t &dataSize)
{
    return g_plugin->retrieve(cacheId, data, dataSize);
}

extern "C" void CacheFreeMemory(void *data, size_t /*dataSize*/)
{
    return g_plugin->free_memory(data);
}
