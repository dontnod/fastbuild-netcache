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

#include "webdav-client.h"

#include <format> // for std::format()

httplib::Result webdav_client::options(std::filesystem::path const &path)
{
    return wrap_request([&](httplib::Client &client)
    {
        return client.Options(path.generic_string());
    });
}

httplib::Result webdav_client::get(std::filesystem::path const &path)
{
    return wrap_request([&](httplib::Client &client)
    {
        return client.Get(path.generic_string());
    });
}

httplib::Result webdav_client::put(std::filesystem::path const &path, void const *data, size_t size)
{
    return wrap_request([&](httplib::Client &client)
    {
        return client.Put(path.generic_string(), static_cast<char const *>(data),
                          size, "application/octet-stream");
    });
}

httplib::Result webdav_client::propfind(std::filesystem::path const &path, std::string const &depth)
{
    httplib::Request req;
    req.method = "PROPFIND";
    req.path = path.generic_string();
    req.headers.insert({"Depth", depth});
    return wrap_request([&](httplib::Client &client)
    {
        return client.send(std::move(req));
    });
}

httplib::Result webdav_client::mkcol(std::filesystem::path const &path)
{
    httplib::Request req;
    req.method = "MKCOL";
    req.path = path.generic_string();
    return wrap_request([&](httplib::Client &client)
    {
        return client.send(std::move(req));
    });
}

void webdav_client::set_basic_auth(std::string const &user, std::string const &pass)
{
    m_user = user;
    m_pass = pass;
}

httplib::Result webdav_client::wrap_request(std::function<httplib::Result(httplib::Client &)> fn)
{
    auto client = get_client();
    auto res = fn(*client);
    if (res && res->status == httplib::StatusCode::Unauthorized_401)
    {
        client->set_basic_auth(m_user, m_pass);
        res = fn(*client);
    }
    return res;
}

std::shared_ptr<httplib::Client> webdav_client::get_client()
{
    auto it = m_pool.find(std::this_thread::get_id());
    if (it != m_pool.end())
        return it->second;

    auto client = std::make_shared<httplib::Client>(m_url);
    client->set_default_headers({
        { "User-Agent", std::format("FASTBuild-NetCache/{}", VERSION) },
    });

    std::unique_lock<std::mutex> lock(m_mutex);
    m_pool.insert({std::this_thread::get_id(), client});
    return client;
}
