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

#include <format> // for std::format()
#include <string> // for std::string
#include <functional> // for std::function
#include <filesystem> // for std::filesystem::path

//
// The abstract cache class
//

class cache
{
public:
    virtual ~cache() = default;

    // Initialise the cache
    virtual bool init(std::string const &cache_root) = 0;

    // Publish a cache entry
    virtual bool publish(std::filesystem::path const &path, std::string_view data) = 0;

    // Retrieve a cache entry
    virtual std::shared_ptr<std::string> retrieve(std::filesystem::path const &path) = 0;

    // Output a message using the std::format syntax
    template<typename... T>
    static void log(std::format_string<T...> const &fmt, T&&... args)
    {
        extern std::function<void(char const *)> g_output_func;
        g_output_func((" - NetCache: " + std::format(fmt, std::forward<T>(args)...)).c_str());
    }
};
