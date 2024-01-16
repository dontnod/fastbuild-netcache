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

#pragma once

#include <memory> // for std::shared_ptr
#include <string> // for std::string
#include <filesystem> // for std::filesystem::path

#include "cache.h"

//
// The network cache class
//

class netcache : public cache
{
public:
    // Initialise the network cache plugin
    virtual bool init(std::string const &cache_root);

    // Publish a cache entry
    virtual bool publish(std::filesystem::path const &path, std::string_view data);

    // Retrieve a cache entry
    virtual std::shared_ptr<std::string> retrieve(std::filesystem::path const &path);

protected:
    // Ensure that a given remote directory exists
    bool ensure_directory(std::filesystem::path path);

private:
    // Path to the cache root on the server
    std::filesystem::path m_root;

    // HTTP/WebDAV client
    std::shared_ptr<class webdav_client> m_client;
};
