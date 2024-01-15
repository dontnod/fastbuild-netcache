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
        plugin::log("directory {} does not exist", cache_root);
        return false;
    }

    plugin::log("initialised file cache for {}", cache_root);
    return true;
}

bool filecache::publish(std::filesystem::path const &path, void const *data, size_t data_size)
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
    file.write(static_cast<char const *>(data), data_size);
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

bool filecache::retrieve(std::filesystem::path const &path, void * &data, size_t &data_size)
{
    std::ifstream file(m_root / path, std::ios::in | std::ios::binary);
    if (!file)
    {
        return false;
    }

    data_size = std::filesystem::file_size(m_root / path);
    std::vector<char> buffer(data_size);
    file.read(buffer.data(), data_size);
    if (file.fail())
    {
        return false;
    }

    data = m_datastore.add(std::move(buffer));
    return true;
}

void filecache::free_memory(void *data)
{
    m_datastore.remove(data);
}
