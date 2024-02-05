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

#include <chrono> // for std::chrono
#include <format> // for std::format
#include <memory> // for std::shared_ptr
#include <mutex>  // for std::mutex()
#include <unordered_set> // for std::unordered_set

//
// The stats tracking class
//

class stats
{
public:
    // An opaque token type for clients
    class token : private std::chrono::duration<float>
    {
        friend class stats;
    };

    // Start tracking time
    std::shared_ptr<token> start()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        sync();
        return *m_tokens.insert(std::make_shared<token>()).first;
    }

    // Stop tracking time and return the final duration
    void stop(std::shared_ptr<token> t, bool hit, size_t bytes)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        sync();
        m_tokens.erase(t);

        m_time += *t;
        m_seen += 1;
        if (hit)
        {
            m_hits += 1;
            m_bytes += bytes;
        }
    }

    std::string summary() const
    {
        return std::format("{: <5} {: <5} {: <5} {:9.2f}  {:7.2f}  {:9.2f}",
                           m_seen, m_hits, m_seen - m_hits, m_bytes / float(1 << 20),
                           m_hits ? m_bytes / float(1 << 20) / m_hits : 0.0f,
                           m_time.count() ? m_bytes / float(1 << 20) / m_time.count() : 0.0f);
    }

protected:
    // Create a synchronisation point for all time tracking tokens, dividing
    // the elapsed time by the number of concurrent timers. This will give us
    // a slightly more accurate estimation of the time spent.
    void sync()
    {
        auto now = std::chrono::high_resolution_clock::now();

        for (auto &timer : m_tokens)
            *timer += (now - m_last_sync) / m_tokens.size();

        m_last_sync = now;
    }

private:
    // Time of the last token creation or deletion
    std::chrono::high_resolution_clock::time_point m_last_sync;

    // List of created time tracking tokens
    std::unordered_set<std::shared_ptr<token>> m_tokens;

    // Total time spent on tokens
    std::chrono::duration<float> m_time;

    // Number of seen, hits, and total bytes
    size_t m_seen, m_hits, m_bytes;

    // Protect stats against concurrent writes
    std::mutex m_mutex;
};
