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
// The plugin class
//

class plugin
{
public:
    // Initialise the plugin
    bool init(std::string const &path);

    // Shut down the plugin
    void shutdown();

    // Publish a cache entry
    bool publish(std::string const &id, std::string_view data);

    // Retrieve a cache entry
    bool retrieve(std::string const &id, void * &data, size_t &data_size);

    // Free the memory held by a cache entry
    void free(void *data);

protected:
    // Convert a cache ID to a sharded filesystem path
    static std::filesystem::path id_to_path(std::string const &id)
    {
        return std::filesystem::path(id.substr(0, 2)) / id.substr(2, 2) / id;
    }

    // All the initialised cache backends
    std::vector<std::shared_ptr<cache>> m_caches;

    // Map of tracked resources
    std::unordered_map<void *, std::shared_ptr<std::string>> m_resources;

    // Protect m_resources against concurrent writes
    std::mutex m_mutex;
};
