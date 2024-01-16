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

bool netcache::init(std::string const &cache_root)
{
    auto match_webdav = std::regex("\\\\\\\\([^\\\\@]*)(@ssl)?(@[0-9]+)?(\\\\(davwwwroot\\\\)?.*)",
                                   std::regex_constants::icase);
    auto match_http = std::regex("^(https?://)([^/:]*)(:[0-9]+)?(.*)$");

    std::string proto, server, port;
    std::smatch m;
    // Split the cache path into protocol (HTTP/HTTPS), server, and path
    if (std::regex_match(cache_root, m, match_webdav))
    {
        proto = m[2].str().empty() ? "http://" : "https://";
        server = m[1].str();
        port = m[3].str();
        std::ranges::replace(port, '@', ':');
        m_root = std::filesystem::path(m[4].str());
    }
    else if (std::regex_match(cache_root, m, match_http))
    {
        proto = m[1].str();
        server = m[2].str();
        port = m[3].str();
        m_root = std::filesystem::path(m[4].str());
    }
    else
    {
        cache::log("unrecognised URL format {}", cache_root);
        return false;
    }

    m_client = std::make_shared<webdav_client>(proto + server + port);

    // Use credentials for the remote server if any are available
    auto user = std::getenv("FASTBUILD_CACHE_USERNAME");
    auto pass = std::getenv("FASTBUILD_CACHE_PASSWORD");
    if (user && user[0] && pass && pass[0])
    {
        cache::log("found environment credentials for user {}", user);
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

            cache::log("found stored credentials for user {}", cred->UserName);
            m_client->set_basic_auth(cred->UserName, pass);
        }
    }
#endif

    // Attempt to connect and possibly authenticate to check that everything is working
    cache::log("testing connection to {}", proto + server + port + m_root.generic_string());
    auto res = m_client->options(m_root);
    if (!res)
    {
        cache::log("cannot query {} ({})", cache_root, httplib::to_string(res.error()));
        return false;
    }
    else if (res->status != httplib::StatusCode::OK_200)
    {
        cache::log("cannot access {} (Status {})", cache_root, res->status);
        return false;
    }

    cache::log("initialised network cache for {}", cache_root);
    return true;
}

bool netcache::publish(std::filesystem::path const &path, std::string_view data)
{
    if (!ensure_directory(path.parent_path()))
    {
        return false;
    }

    auto res = m_client->put(m_root / path, data.data(), data.length());
    if (!res || res->status != httplib::StatusCode::Created_201)
    {
        return false;
    }

    return true;
}

std::shared_ptr<std::string> netcache::retrieve(std::filesystem::path const &path)
{
    auto res = m_client->get(m_root / path);
    if (!res || res->status != httplib::StatusCode::OK_200)
    {
        return nullptr;
    }

    return std::make_shared<std::string>(std::move(res->body));
}

bool netcache::ensure_directory(std::filesystem::path path)
{
    // Return true if the directory exists
    auto res = m_client->propfind(m_root / path, "0");
    if (res && res->status == httplib::StatusCode::MultiStatus_207)
    {
        return true;
    }

    // Otherwise, try to create it, but only after ensuring the parent directory exists
    if (res && res->status == httplib::StatusCode::NotFound_404
            && path.has_parent_path() && ensure_directory(path.parent_path()))
    {
        res = m_client->mkcol(m_root / path);
        return res && res->status == httplib::StatusCode::Created_201;
    }

    return false;
}
