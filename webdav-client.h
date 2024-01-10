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

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "3rdparty/cpp-httplib/httplib.h"

#include <format> // for std::format()
#include <memory> // for std::shared_ptr
#include <string> // for std::string
#include <filesystem> // for std::filesystem::path
#include <unordered_map> // for std::unordered_map

//
// HTTP/WebDAV client that creates a new connection for each calling thread
//

class webdav_client
{
public:
    webdav_client(std::string const &url)
      : m_url(url)
    {
    }

    httplib::Result options(std::filesystem::path const &path)
    {
        return get()->Options(path.generic_string());
    }

    httplib::Result get(std::filesystem::path const &path)
    {
        return get()->Get(path.generic_string());
    }

    httplib::Result put(std::filesystem::path const &path, void const *data, size_t size)
    {
        return get()->Put(path.generic_string(), static_cast<char const *>(data), size,
                          "application/octet-stream");
    }

    void set_basic_auth(std::string const &user, std::string const &pass)
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_user = user;
        m_pass = pass;

        for (auto &kv : m_pool)
            kv.second->set_basic_auth(m_user, m_pass);
    }

protected:
    // Return the current thread’s web client, or create one if necessary
    std::shared_ptr<httplib::Client> get()
    {
        auto it = m_pool.find(std::this_thread::get_id());
        if (it != m_pool.end())
            return it->second;

        auto client = std::make_shared<httplib::Client>(m_url);
        client->set_default_headers({
            { "User-Agent", std::format("FASTBuild-NetCache/{}", VERSION) },
        });
        client->set_basic_auth(m_user, m_pass);

        std::unique_lock<std::mutex> lock(m_mutex);
        m_pool.insert({std::this_thread::get_id(), client}).second;
        return client;
    }

private:
    // Base URL to connect to (protocol, server name, port)
    std::string m_url;

    // Network cache credentials, if any
    std::string m_user, m_pass;

    // Per-thread collection of web clients
    std::unordered_map<std::thread::id, std::shared_ptr<httplib::Client>> m_pool;

    // Protect m_user, m_pass, and m_pool against concurrent writes
    std::mutex m_mutex;
};
