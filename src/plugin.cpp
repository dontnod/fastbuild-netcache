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

#include <algorithm> // for std::find_if()
#include <memory>    // for std::shared_ptr
#include <mutex>     // for std::mutex
#include <sstream>   // for std::stringstream
#include <string>    // for std::string, std::getline()
#include <unordered_map> // for std::unordered_map
#include <vector>    // for std::vector

#include "plugin.h"
#include "filecache.h"
#include "netcache.h"

// Global variable storing the plugin instance; the current API does not allow
// to track a state or a closure, so this has to be global.
static plugin g_plugin;

// Global variable storing the logging function provided by FASTBuild
std::function<void(char const *)> g_output_func;

bool plugin::init(std::string const &path)
{
    std::stringstream ss(path);
    for (std::string path; std::getline(ss, path, ';'); )
    {
        // Try to initialise a network cache, or fall back to a file cache
        if (auto cache = std::make_shared<netcache>(); cache->init(path))
        {
            m_caches.push_back(cache);
        }
        else if (auto cache = std::make_shared<filecache>(); cache->init(path))
        {
            m_caches.push_back(cache);
        }
    }

    // Succeed if at least one cache could be created
    return !m_caches.empty();
}

void plugin::shutdown()
{
    m_caches.clear();
}

bool plugin::publish(std::string const &id, std::string_view data)
{
    // Publish to all caches
    return std::all_of(m_caches.begin(), m_caches.end(), [&](auto &cache) {
        return cache->publish(id_to_path(id), data);
    });
}

bool plugin::retrieve(std::string const &id, void * &data, size_t &data_size)
{
    // Try all caches until we find our data
    for (auto cache : m_caches)
    {
        if (auto buffer = cache->retrieve(id_to_path(id)); buffer)
        {
            data = buffer->data();
            data_size = buffer->size();

            std::unique_lock<std::mutex> lock(m_mutex);
            return m_resources.insert({data, buffer}).second;
        }
    }

    return false;
}

void plugin::free(void *data)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_resources.erase(data);
}

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
    g_output_func = outputFunc;
    return g_plugin.init(cachePath);
}

extern "C" void CacheShutdown()
{
    g_plugin.shutdown();
}

extern "C" bool CachePublish(char const *cacheId, char const *data, size_t dataSize)
{
    return g_plugin.publish(cacheId, std::string_view(data, data + dataSize));
}

extern "C" bool CacheRetrieve(char const *cacheId, void * &data, size_t &dataSize)
{
    return g_plugin.retrieve(cacheId, data, dataSize);
}

extern "C" void CacheFreeMemory(void *data, size_t /*dataSize*/)
{
    g_plugin.free(data);
}
