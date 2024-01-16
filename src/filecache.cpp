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

#include <fstream> // for std::[io]fstream
#include <random>  // for std::minstd_rand

#include "filecache.h"

bool filecache::init(std::string const &cache_root)
{
    m_root = std::filesystem::path(cache_root);
    if (!std::filesystem::is_directory(m_root))
    {
        cache::log("directory {} does not exist", cache_root);
        return false;
    }

    cache::log("initialised file cache for {}", cache_root);
    return true;
}

bool filecache::publish(std::filesystem::path const &path, std::string_view data)
{
    static std::minstd_rand rand;

    std::error_code ec;
    std::filesystem::path tmp = m_root / path;
    tmp += std::format(".tmp{:06x}", rand() & 0xffffff);

    // Ensure target directory exists
    std::filesystem::create_directories(m_root / path.parent_path(), ec);
    if (ec.value() != 0)
    {
        return false;
    }

    // Open temporary file for writing
    std::ofstream file(tmp, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file)
    {
        return false;
    }

    // Write data; in case of failure, remove the temporary file
    file.write(data.data(), data.length());
    if (file.fail())
    {
        std::filesystem::remove(tmp, ec);
        return false;
    }
    file.close();

    // Move temporary file to destination; in case of failure, remove the temporary file
    std::filesystem::rename(tmp, m_root / path, ec);
    if (ec.value() != 0)
    {
        std::filesystem::remove(tmp, ec);
        return false;
    }

    return true;
}

std::shared_ptr<std::string> filecache::retrieve(std::filesystem::path const &path)
{
    std::ifstream file(m_root / path, std::ios::in | std::ios::binary);
    if (!file)
    {
        return nullptr;
    }

    auto size = std::filesystem::file_size(m_root / path);
    auto buffer = std::make_shared<std::string>(size, '\0');
    file.read(buffer->data(), size);
    if (file.fail())
    {
        return nullptr;
    }

    return buffer;
}
