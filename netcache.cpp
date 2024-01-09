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
#include <algorithm> // for std::ranges::replace()
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
        static auto match_webdav = std::regex("\\\\\\\\([^\\\\@]*)(@ssl)?(\\\\(davwwwroot\\\\)?.*[^\\\\])\\\\*",
                                              std::regex_constants::icase);
        static auto match_http = std::regex("^(https?://)([^/]*)(.*[^/])/*$");

        std::cmatch m;
        // Split the cache path into protocol (HTTP/HTTPS), server, and path (without trailing slash)
        if (std::regex_match(cachePath, m, match_webdav))
        {
            m_proto = m[2].str().size() ? "https://" : "http://";
            m_server = m[1].str();
            m_prefix = m[3].str();
            std::ranges::replace(m_prefix, '\\', '/');
        }
        else if (std::regex_match(cachePath, m, match_http))
        {
            m_proto = m[1].str();
            m_server = m[2].str();
            m_prefix = m[3].str();
        }
        else
        {
            output("unrecognised URL format {}, disabling netcache", cachePath);
            return false;
        }

        // Attempt to connect and possibly authenticate to check that everything is working
        auto ret = http_client()->Options(m_prefix);
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
        auto res = http_client()->Put(shard(cacheId), static_cast<char const *>(data),
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
        auto res = http_client()->Get(shard(cacheId));
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
    // Return the current thread’s web client, or create one if necessary
    std::shared_ptr<httplib::Client> http_client()
    {
        auto it = m_web_clients.find(std::this_thread::get_id());
        if (it != m_web_clients.end())
            return it->second;

        auto client = std::make_shared<httplib::Client>(m_proto + m_server);
        client->set_default_headers({
            { "User-Agent", std::format("FASTBuild-NetCache/{}", VERSION) },
        });

        std::unique_lock<std::mutex> lock(m_mutex);

#if _WIN32
        if (m_user.empty() && m_pass.empty())
        {
            // If applicable, set basic auth information using stored Windows credentials
            PCREDENTIALA cred;
            if (CredReadA(m_server.c_str(), CRED_TYPE_GENERIC, 0, &cred) == TRUE)
            {
                // User is stored in 8-byte chars
                m_user.assign(cred->UserName);

                // Password is stored as UTF-16 so we have to convert it to UTF-8.
                int len = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)cred->CredentialBlob, (int)cred->CredentialBlobSize,
                                              nullptr, 0, nullptr, nullptr);
                m_pass.resize(len);
                WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)cred->CredentialBlob, (int)cred->CredentialBlobSize,
                                    (LPSTR)m_pass.data(), (int)m_pass.size(), nullptr, nullptr);

                output("found stored credentials for user {}", m_user);
            }
        }
#endif

        if (!m_user.empty() || !m_pass.empty())
            client->set_basic_auth(m_user, m_pass);

        m_web_clients.insert({std::this_thread::get_id(), client}).second;
        return client;
    }

    // Output a message using the std::format syntax
    template<typename... T> void output(std::format_string<T...> const &fmt, T&&... args)
    {
        m_output_func((" - NetCache: " + std::format(fmt, std::forward<T>(args)...)).c_str());
    }

    // Convert a cacheId value to a full path on the server
    std::string shard(char const *cacheId)
    {
        return std::format("{}/{:.2s}/{:.2s}/{}", m_prefix, cacheId, cacheId + 2, cacheId);
    }

    // Network cache information: protocol, server name, path prefix
    std::string m_proto, m_server, m_prefix;

    // Network cache credentials, if any
    std::string m_user, m_pass;

    // Per-thread collection of web clients
    std::unordered_map<std::thread::id, std::shared_ptr<httplib::Client>> m_web_clients;

    // Map of response data for resource tracking
    std::unordered_map<void *, std::shared_ptr<std::string>> m_data;

    // Protect m_user, m_pass, m_web_clients and m_data against concurrent writes
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
