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

#include "cache.h"

// Initialise the cache
bool cache::init(std::string const &cache_root)
{
    m_root = cache_root;
    return init_internal(cache_root);
}

// Publish a cache entry
bool cache::publish(std::filesystem::path const &path, std::string_view data)
{
    auto timer = m_publish.start();
    auto ret = publish_internal(path, data);
    m_publish.stop(timer, ret, data.size());
    return ret;
}

// Retrieve a cache entry
std::shared_ptr<std::string> cache::retrieve(std::filesystem::path const &path)
{
    auto timer = m_retrieve.start();
    auto ret = retrieve_internal(path);
    m_retrieve.stop(timer, bool(ret), ret ? ret->size() : 0);
    return ret;
}

// Print stats about the cache
void cache::summary() const
{
    extern std::function<void(char const *)> g_output_func;
    g_output_func(std::format(" - {}", m_root).c_str());
    g_output_func(std::format(" - Retrieve  : {}", m_retrieve.summary()).c_str());
    g_output_func(std::format(" - Publish   : {}", m_publish.summary()).c_str());
}
