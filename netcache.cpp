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
#include "3rdparty/fastbuild/Code/Tools/FBuild/FBuildCore/Cache/CachePluginInterface.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "3rdparty/cpp-httplib/httplib.h"

#include "webdav-client.h"

#if _WIN32
#   include <wincred.h>      // for CredReadA()
#   include <stringapiset.h> // for WideCharToMultiByte()
#endif

#include <format> // for std::format()
#include <memory> // for std::shared_ptr
#include <regex>  // for std::regex_match()
#include <string> // for std::string
#include <algorithm> // for std::ranges::replace()
#include <filesystem> // for std::filesystem::path
#include <unordered_map> // for std::unordered_map

//
// Global variable storing the cache plugin data; the current API does not allow
// to track a state or a closure, so this has to be global.
//

static std::shared_ptr<class netcache> g_plugin;

//
// The network cache class
//

class netcache
{
public:
    netcache(CacheOutputFunc outputFunc)
      : m_output_func(outputFunc)
    {}

    // Initialise the network cache plugin
    bool init(char const *cachePath, bool cacheRead, bool cacheWrite, bool cacheVerbose, char const *userConfig)
    {
        auto match_webdav = std::regex("\\\\\\\\([^\\\\@]*)(@ssl)?(@[0-9]+)?(\\\\(davwwwroot\\\\)?.*[^\\\\])\\\\*",
                                       std::regex_constants::icase);
        auto match_http = std::regex("^(https?://)([^/:]*)(:[0-9]+)?(.*[^/])/*$");

        std::string proto, server, port;
        std::cmatch m;
        // Split the cache path into protocol (HTTP/HTTPS), server, and path (without trailing slash)
        if (std::regex_match(cachePath, m, match_webdav))
        {
            proto = m[2].str().empty() ? "http://" : "https://";
            server = m[1].str();
            port = m[3].str();
            std::ranges::replace(port, '@', ':');
            m_root = std::filesystem::path(m[4].str());
        }
        else if (std::regex_match(cachePath, m, match_http))
        {
            proto = m[1].str();
            server = m[2].str();
            port = m[3].str();
            m_root = std::filesystem::path(m[4].str());
        }
        else
        {
            output("unrecognised URL format {}, disabling netcache", cachePath);
            return false;
        }

        // Attempt to connect and possibly authenticate to check that everything is working
        m_client = std::make_shared<webdav_client>(proto + server + port);

#if _WIN32
        // If applicable, set basic auth information using stored Windows credentials
        PCREDENTIALA cred;
        if (CredReadA(server.c_str(), CRED_TYPE_GENERIC, 0, &cred) == TRUE)
        {
            // User is stored as 8-bit chars but password is stored as UTF-16. Convert it to UTF-8.
            int len = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)cred->CredentialBlob, (int)cred->CredentialBlobSize,
                                          nullptr, 0, nullptr, nullptr);
            std::string pass(len, '\0');
            WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)cred->CredentialBlob, (int)cred->CredentialBlobSize,
                                (LPSTR)pass.data(), (int)pass.size(), nullptr, nullptr);

            output("found stored credentials for user {}", cred->UserName);
            m_client->set_basic_auth(cred->UserName, pass);
        }
#endif

        output("testing connection to {}", proto + server + port + m_root.generic_string());
        auto ret = m_client->options(m_root);
        if (ret.error() != httplib::Error::Success)
        {
            output("cannot query {} ({}), disabling cache", cachePath, httplib::to_string(ret.error()));
            return false;
        }
        else if (ret->status != httplib::StatusCode::OK_200)
        {
            output("cannot access {} (Status {}), disabling cache", cachePath, ret->status);
            return false;
        }

        output("initialised cache for {}", cachePath);
        return true;
    }

    // Publish a cache entry
    bool publish(char const *cacheId, const void *data, size_t dataSize)
    {
        auto res = m_client->put(shard(cacheId), data, dataSize);
        if (res->status != httplib::StatusCode::Created_201)
        {
            return false;
        }

        return true;
    }

    // Retrieve a cache entry
    bool retrieve(char const *cacheId, void * &data, size_t &dataSize)
    {
        auto res = m_client->get(shard(cacheId));
        if (res->status != httplib::StatusCode::OK_200)
        {
            return false;
        }

        // Wrap the response inside a shared pointer so we can safely store it
        // in a private map for later releasing.
        auto body = std::make_shared<std::string>(std::move(res->body));
        data = body->data();
        dataSize = body->size();

        std::unique_lock<std::mutex> lock(m_mutex);
        return m_data.insert({data, body}).second;
    }

    // Free memory allocated by a previous retrieve() call
    void free_memory(void *data)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_data.erase(data);
    }

protected:
    // Output a message using the std::format syntax
    template<typename... T> void output(std::format_string<T...> const &fmt, T&&... args)
    {
        m_output_func((" - NetCache: " + std::format(fmt, std::forward<T>(args)...)).c_str());
    }

    // Convert a cacheId value to a full path on the server
    std::filesystem::path shard(char const *cacheId)
    {
        return m_root / std::string(cacheId, 2) / std::string(cacheId + 2, 2) / cacheId;
    }

private:
    // Path to the cache root on the server
    std::filesystem::path m_root;

    // HTTP/WebDAV client
    std::shared_ptr<webdav_client> m_client;

    // Map of response data for resource tracking
    std::unordered_map<void *, std::shared_ptr<std::string>> m_data;

    // Protect m_data against concurrent writes
    std::mutex m_mutex;

    // Logging function provided by FASTBuild
    CacheOutputFunc m_output_func;
};

//
// FASTBuild cache plugin API implementation
//

extern "C" bool CacheInitEx(const char *cachePath,
                            bool cacheRead,
                            bool cacheWrite,
                            bool cacheVerbose,
                            const char *userConfig,
                            CacheOutputFunc outputFunc)
{
    g_plugin = std::make_shared<netcache>(outputFunc);
    return g_plugin->init(cachePath, cacheRead, cacheWrite, cacheVerbose, userConfig);
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
