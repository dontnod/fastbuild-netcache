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

#include <format> // for std::format()
#include <memory> // for std::shared_ptr
#include <string> // for std::string
#include <thread> // for std::mutex
#include <functional> // for std::function
#include <filesystem> // for std::filesystem::path
#include <unordered_map> // for std::unordered_map

//
// The network cache class
//

class netcache
{
public:
    netcache(std::function<void(char const *)> output_func)
      : m_output_func(output_func)
    {}

    // Initialise the network cache plugin
    bool init(std::string const &cache_root);

    // Publish a cache entry
    bool publish(std::filesystem::path const &path, const void *data, size_t dataSize);

    // Retrieve a cache entry
    bool retrieve(std::filesystem::path const &path, void * &data, size_t &dataSize);

    // Free memory allocated by a previous retrieve() call
    void free_memory(void *data);

protected:
    // Output a message using the std::format syntax
    template<typename... T> void output(std::format_string<T...> const &fmt, T&&... args)
    {
        m_output_func((" - NetCache: " + std::format(fmt, std::forward<T>(args)...)).c_str());
    }

    // Ensure that a given remote directory exists
    bool ensure_directory(std::filesystem::path path);

private:
    // Path to the cache root on the server
    std::filesystem::path m_root;

    // HTTP/WebDAV client
    std::shared_ptr<class webdav_client> m_client;

    // Map of response data for resource tracking
    std::unordered_map<void *, std::shared_ptr<std::string>> m_data;

    // Protect m_data against concurrent writes
    std::mutex m_mutex;

    // Logging function provided by FASTBuild
    std::function<void(char const *)> m_output_func;
};
