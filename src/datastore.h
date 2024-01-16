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
#include <mutex>  // for std::mutex
#include <unordered_map> // for std::unordered_map

template<typename T> class datastore
{
public:
    // Keep track of a resource (typically std::string or std::vector) and return
    // its data pointer for later releasing
    bool add(std::shared_ptr<T> p)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_data.insert({p->data(), p}).second;
    }

    // Release a resource identified by its data pointer
    void remove(void *data)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_data.erase(data);
    }

protected:
    // Map of tracked resources
    std::unordered_map<void *, std::shared_ptr<T>> m_data;

    // Protect m_data against concurrent writes
    std::mutex m_mutex;
};
