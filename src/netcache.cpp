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
#   include <windef.h>       // for DWORD, LPSTR, etc.
#   include <wincred.h>      // for CredReadA()
#   include <stringapiset.h> // for WideCharToMultiByte()
#endif

#include <regex>  // for std::regex_match()
#include <cstdlib> // for std::getenv()
#include <algorithm> // for std::ranges::replace()

#include "netcache.h"
#include "webdav-client.h"

bool netcache::init(char const *cachePath)
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

    m_client = std::make_shared<webdav_client>(proto + server + port);

    // Use credentials for the remote server if any are available
    auto user = std::getenv("FASTBUILD_CACHE_USERNAME");
    auto pass = std::getenv("FASTBUILD_CACHE_PASSWORD");
    if (user && user[0] && pass && pass[0])
    {
        output("found environment credentials for user {}", user);
        m_client->set_basic_auth(user, pass);
    }
#if _WIN32
    else
    {
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
    }
#endif

    // Attempt to connect and possibly authenticate to check that everything is working
    output("testing connection to {}", proto + server + port + m_root.generic_string());
    auto res = m_client->options(m_root);
    if (!res)
    {
        output("cannot query {} ({}), disabling cache", cachePath, httplib::to_string(res.error()));
        return false;
    }
    else if (res->status != httplib::StatusCode::OK_200)
    {
        output("cannot access {} (Status {}), disabling cache", cachePath, res->status);
        return false;
    }

    output("initialised cache for {}", cachePath);
    return true;
}

bool netcache::publish(char const *cacheId, const void *data, size_t dataSize)
{
    auto path = shard(cacheId);
    if (!ensure_directory(path.parent_path()))
    {
        return false;
    }

    auto res = m_client->put(path, data, dataSize);
    if (!res || res->status != httplib::StatusCode::Created_201)
    {
        return false;
    }

    return true;
}

bool netcache::retrieve(char const *cacheId, void * &data, size_t &dataSize)
{
    auto res = m_client->get(shard(cacheId));
    if (!res || res->status != httplib::StatusCode::OK_200)
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

void netcache::free_memory(void *data)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_data.erase(data);
}

bool netcache::ensure_directory(std::filesystem::path path)
{
    // Return true if the directory exists
    auto res = m_client->propfind(path, "0");
    if (res && res->status == httplib::StatusCode::MultiStatus_207)
    {
        return true;
    }

    // Otherwise, try to create it, but only after ensuring the parent directory exists
    if (res && res->status == httplib::StatusCode::NotFound_404
            && path.has_parent_path() && ensure_directory(path.parent_path()))
    {
        res = m_client->mkcol(path);
        return res && res->status == httplib::StatusCode::Created_201;
    }

    return false;
}

std::filesystem::path netcache::shard(char const *cacheId)
{
    return m_root / std::string(cacheId, 2) / std::string(cacheId + 2, 2) / cacheId;
}