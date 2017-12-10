/*
 * Copyright (c) 2017, Yutaka Tsutano
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef ETSL_ALGORITHM_HPP
#define ETSL_ALGORITHM_HPP

namespace etsl {
    template <typename T>
    void unique_sort(std::vector<T>& vec)
    {
        std::sort(begin(vec), end(vec));
        vec.erase(std::unique(begin(vec), end(vec)), end(vec));
    }

    void trim_inplace(std::string& s)
    {
        auto from = begin(s);
        auto to = from;

        for (; from != end(s) && std::isspace(*from); ++from) {
        }

        for (; from != end(s); ++from, ++to) {
            *to = *from;
        }

        s.erase(to, end(s));

        while (!s.empty() && std::isspace(s.back())) {
            s.pop_back();
        }
    }
}

#endif
