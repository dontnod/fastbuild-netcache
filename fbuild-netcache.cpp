//
//  FASTBuild Network Cache Plugin
//
//  Copyright © 2024 Don’t Nod Entertainment S.A. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining
//  a copy of this software and associated documentation files (the
//  "Software"), to deal in the Software without restriction, including
//  without limitation the rights to use, copy, modify, merge, publish,
//  distribute, sublicense, and/or sell copies of the Software, and to
//  permit persons to whom the Software is furnished to do so, subject to
//  the following conditions:
//
//  The above copyright notice and this permission notice shall be
//  included in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
//  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
//  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#if _WIN32
#   define __WINDOWS__ 1
#elif __linux__
#   define __LINUX__ 1
#endif
#include "CachePluginInterface.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

#if _WIN32
#   include <wincred.h>      // for CredReadA()
#   include <stringapiset.h> // for WideCharToMultiByte()
#endif

#include <format> // for std::format()
#include <memory> // for std::shared_ptr
#include <regex>  // for std::regex_match()
#include <string> // for std::string
#include <unordered_map> // for std::unordered_map

//
// Global variable storing the cache plugin data; the current API does not allow
// to track a state or a closure, so this has to be global.
//

static std::shared_ptr<class fbuild_netcache> g_plugin;

//
// The network cache class
//

class fbuild_netcache
{
public:
    fbuild_netcache(CacheOutputFunc outputFunc)
      : m_output_func(outputFunc)
    {}

    // Initialise the network cache plugin
    bool init(char const *cachePath, bool cacheRead, bool cacheWrite, bool cacheVerbose, char const *userConfig)
    {
        // Split the cache path into the protocol+server part, and the rest (dropping the trailing slash)
        std::cmatch m;
        if (!std::regex_match(cachePath, m, std::regex("^(https?://)([^/]*)(.*[^/])/*$")))
        {
            output(" - Cache: unrecognised URL format {}, disabling netcache", cachePath);
            return false;
        }

        m_proto = m[1].str();
        m_server = m[2].str();
        m_prefix = m[3].str();

        // Create a web client using the server part
        m_web_client = std::make_shared<httplib::Client>(m_proto + m_server);
        m_web_client->set_default_headers({
            { "User-Agent", std::format("FASTBuild Network Cache Plugin v{}", VERSION) },
        });

#if _WIN32
        // If applicable, set basic auth information using stored Windows credentials
        PCREDENTIALA cred;
        if (CredReadA(m_server.c_str(), CRED_TYPE_GENERIC, 0, &cred) == TRUE)
        {
            // Password is stored as UTF-16 so we have to convert it to UTF-8.
            int len = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)cred->CredentialBlob, (int)cred->CredentialBlobSize,
                                          nullptr, 0, nullptr, nullptr);
            std::string pwd(len, '\0');
            WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)cred->CredentialBlob, (int)cred->CredentialBlobSize,
                                (LPSTR)pwd.data(), (int)pwd.size(), nullptr, nullptr);

            output(" - Cache: found stored credentials for user {}", cred->UserName);
            m_web_client->set_basic_auth(cred->UserName, pwd);
        }
#endif

        // Attempt to connect and possibly authenticate to check that everything is working
        auto ret = m_web_client->Options(m_prefix);
        if (ret.error() != httplib::Error::Success)
        {
            output(" - Cache: cannot query {} ({}), disabling netcache",
                   cachePath, httplib::to_string(ret.error()));
            return false;
        }
        else if (ret->status != httplib::StatusCode::OK_200)
        {
            output(" - Cache: cannot access {} (Status {}), disabling netcache",
                   cachePath, ret->status);
            return false;
        }

        output(" - Cache: initialised netcache for {}", cachePath);
        return true;
    }

    // Publish a cache entry
    bool publish(char const *cacheId, const void *data, size_t dataSize)
    {
        auto res = m_web_client->Put(shard(cacheId), static_cast<char const *>(data),
                                     dataSize, "application/octet-stream");
        if (res->status != httplib::StatusCode::Created_201)
        {
            return false;
        }

        return true;
    }

    // Retrieve a cache entry
    bool retrieve(char const *cacheId, void * &data, size_t &dataSize)
    {
        auto res = m_web_client->Get(shard(cacheId));
        if (res->status != httplib::StatusCode::OK_200)
        {
            return false;
        }

        data = res->body.data();
        dataSize = res->body.size();
        m_data.insert(std::make_pair(data, std::move(res->body)));
        return true;
    }

    // Free memory allocated by a previous retrieve() call
    void free_memory(void *data)
    {
        m_data.erase(data);
    }

protected:
    // Output a message using the std::format syntax
    template<typename... T> void output(std::format_string<T...> const &fmt, T&&... args)
    {
        m_output_func(std::format(fmt, std::forward<T>(args)...).c_str());
    }

    // Convert a cacheId value to a full path on the server
    std::string shard(char const *cacheId)
    {
        return std::format("{}/{:.2s}/{:.2s}/{}", m_prefix, cacheId, cacheId + 2, cacheId);
    }

    std::shared_ptr<httplib::Client> m_web_client;
    std::string m_proto, m_server, m_prefix;
    std::unordered_map<void *, std::string> m_data;
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
    g_plugin = std::make_shared<fbuild_netcache>(outputFunc);
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
