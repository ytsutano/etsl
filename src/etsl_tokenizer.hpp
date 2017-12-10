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

#ifndef ETSL_TOKENIZER_HPP
#define ETSL_TOKENIZER_HPP

#include "etsl_file.hpp"
#include "algorithm.hpp"

namespace etsl {
    struct etsl_syntax_error : std::runtime_error {
        int line_num;
        int col_num;

        etsl_syntax_error(int line_num, int col_num, std::string what)
                : runtime_error(what), line_num(line_num), col_num(col_num)
        {
        }
    };

    struct etsl_token {
        enum {
            kind_unknown,
            kind_category,
            kind_choice,
            kind_attribute
        } kind = kind_unknown;
        std::string str;
        int line_num = 0;
        int col_num = 0;
    };

    std::vector<etsl_token> etsl_tokenize(std::istream& is)
    {
        std::vector<etsl_token> tokens(1);

        int line_num = 1;
        int col_num = 0;

        bool in_constraints = false;

        char c;
        while (is.get(c)) {
            if (c == '\r') {
                continue;
            }

            ++col_num;

            switch (c) {
            case '\n':
                ++line_num;
                col_num = 0;
                continue;
            case '#':
                // Ignore a comment.
                is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                ++line_num;
                col_num = 0;
                break;
            case '[':
                in_constraints = true;
                break;
            case ']':
                tokens.back().kind = etsl_token::kind_attribute;
                tokens.back().line_num = line_num;
                tokens.back().col_num = col_num;
                tokens.emplace_back();
                in_constraints = false;
                break;
            case ':':
                if (!in_constraints) {
                    tokens.back().kind = etsl_token::kind_category;
                    tokens.back().line_num = line_num;
                    tokens.back().col_num = col_num;
                    tokens.emplace_back();
                }
                else {
                    tokens.back().str.push_back(c);
                }
                break;
            case '.':
                if (!in_constraints) {
                    tokens.back().kind = etsl_token::kind_choice;
                    tokens.back().line_num = line_num;
                    tokens.back().col_num = col_num;
                    tokens.emplace_back();
                }
                else {
                    tokens.back().str.push_back(c);
                }
                break;
            default:
                tokens.back().str.push_back(c);
            }
        }
        tokens.pop_back();

        for (auto& t : tokens) {
            trim_inplace(t.str);
        }

        return tokens;
    }

    std::vector<std::string> etsl_attr_subtokenize(const etsl_token& token)
    {
        static const char* keywords[]
                = {"if", "else", "property", "single", "error"};

        std::vector<std::string> attr_subtokens(1);

        for (auto it = begin(token.str); it != end(token.str); ++it) {
            if (std::isspace(*it)) {
                if (std::find(std::begin(keywords), std::end(keywords),
                              attr_subtokens.back())
                    != std::end(keywords)) {
                    attr_subtokens.emplace_back();
                }
                else if (!attr_subtokens.back().empty()) {
                    attr_subtokens.back().push_back(*it);
                }
                continue;
            }

            switch (*it) {
            case '(':
            case ')':
            case '!':
            case ',':
                if (!attr_subtokens.back().empty()) {
                    attr_subtokens.emplace_back();
                }
                attr_subtokens.back() = *it;
                attr_subtokens.emplace_back();
                break;
            case '|':
            case '&':
                if (it + 1 == end(token.str) || *it != *(it + 1)) {
                    throw etsl_syntax_error(token.line_num, token.col_num,
                                            "invalid attribute");
                }
                ++it;
                if (!attr_subtokens.back().empty()) {
                    attr_subtokens.emplace_back();
                }
                attr_subtokens.back() = std::string(2, *it);
                attr_subtokens.emplace_back();
                break;
            default:
                attr_subtokens.back().push_back(*it);
            }
        }

        for (auto& at : attr_subtokens) {
            trim_inplace(at);
        }

        if (attr_subtokens.back().empty()) {
            attr_subtokens.pop_back();
        }

        return attr_subtokens;
    }
}

#endif
