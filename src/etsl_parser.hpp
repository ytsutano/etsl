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

#ifndef ETSL_PARSER_HPP
#define ETSL_PARSER_HPP

#include <algorithm>

#include "etsl_file.hpp"
#include "etsl_tokenizer.hpp"
#include "util.hpp"

namespace etsl {
    namespace detail {
        class etsl_parser {
        private:
            etsl_file& file_;
            const std::vector<etsl_token>& tokens_;
            bool mutually_exclusive_choices_ = false;
            enum {
                attr_state_init,
                attr_state_if,
                attr_state_else
            } attr_state_;

        private:
            void parse_category(const etsl_token& token)
            {
                if (!file_.categories.empty()
                    && file_.categories.back().choices.empty()) {
                    if (file_.categories.back().name == "Expectations") {
                        // We are now in the Expectations section.
                        mutually_exclusive_choices_ = true;
                    }
                    file_.categories.pop_back();
                }
                file_.categories.emplace_back(token.str,
                                              mutually_exclusive_choices_);
            }

            void parse_choice(const etsl_token& token)
            {
                if (file_.categories.empty()) {
                    throw etsl_syntax_error(token.line_num, token.col_num,
                                            "unexpected choice");
                }
                auto& category = file_.categories.back();
                category.choices.emplace_back(token.str);
            }

            template <typename F>
            void attr_assert(const etsl_token& token, F pred)
            {
                if (!pred()) {
                    throw etsl_syntax_error(token.line_num, token.col_num,
                                            "invalid attribute expression");
                }
            }

            void parse_attribute(const etsl_token& token)
            {
                if (file_.categories.empty()
                    || file_.categories.back().choices.empty()) {
                    throw etsl_syntax_error(token.line_num, token.col_num,
                                            "unexpected attribute");
                }
                etsl_choice& choice = file_.categories.back().choices.back();

                auto attr_subtokens = etsl_attr_subtokenize(token);
                attr_assert(token, [&] { return !attr_subtokens.empty(); });

                const auto it_end = end(attr_subtokens);
                auto it = begin(attr_subtokens);
                const std::string& keyword = *it;
                ++it;
                if (keyword == "if") {
                    attr_assert(token,
                                [&] { return attr_state_ == attr_state_init; });
                    attr_state_ = attr_state_if;

                    try {
                        choice.cond.parse(it, end(attr_subtokens));
                    }
                    catch (etsl_invalid_predicate_error& ex) {
                        throw etsl_syntax_error(token.line_num, token.col_num,
                                                ex.what());
                    }
                    choice.has_if = true;
                }
                else if (keyword == "else") {
                    attr_assert(token,
                                [&] { return attr_state_ == attr_state_if; });
                    attr_state_ = attr_state_else;
                    choice.has_else = true;
                }
                else if (keyword == "single" || keyword == "error") {
                    attr_assert(token, [&] { return it == it_end; });
                    switch (attr_state_) {
                    case attr_state_init:
                        choice.single_str = keyword;
                        break;
                    case attr_state_if:
                        choice.if_single_str = keyword;
                        break;
                    case attr_state_else:
                        choice.else_single_str = keyword;
                        break;
                    }
                }
                else if (keyword == "property") {
                    attr_assert(token, [&] { return it != it_end; });

                    while (it != it_end) {
                        switch (attr_state_) {
                        case attr_state_init:
                            choice.if_props.push_back(*it);
                            choice.else_props.push_back(*it);
                            break;
                        case attr_state_if:
                            choice.if_props.push_back(*it);
                            break;
                        case attr_state_else:
                            choice.else_props.push_back(*it);
                            break;
                        }
                        ++it;
                        if (it == it_end) {
                            break;
                        }

                        attr_assert(token, [&] { return *it == ","; });
                        ++it;
                    }
                }
            }

        public:
            etsl_parser(etsl_file& file, const std::vector<etsl_token>& tokens)
                    : file_(file), tokens_(tokens)
            {
                for (const auto& t : tokens_) {
                    switch (t.kind) {
                    case etsl_token::kind_category:
                        parse_category(t);
                        break;
                    case etsl_token::kind_choice:
                        parse_choice(t);
                        attr_state_ = attr_state_init;
                        break;
                    case etsl_token::kind_attribute:
                        parse_attribute(t);
                        break;
                    default:
                        throw etsl_syntax_error(t.line_num, t.col_num,
                                                "invalid token");
                    }
                }

                // Add automatic properties.
                for (etsl_category& cat : file_.categories) {
                    for (etsl_choice& ch : cat.choices) {
                        ch.if_props.push_back(cat.name + ":" + ch.name);
                        ch.else_props.push_back(cat.name + ":" + ch.name);
                        ch.if_props.push_back(":" + ch.name);
                        ch.else_props.push_back(":" + ch.name);
                        if (ch.name == "true") {
                            ch.if_props.push_back(cat.name);
                            ch.else_props.push_back(cat.name);
                        }
                    }
                }

                auto unique_sort = [](std::vector<std::string>& vec) {
                    std::sort(begin(vec), end(vec));
                    vec.erase(std::unique(begin(vec), end(vec)), end(vec));
                };
                for (etsl_category& cat : file_.categories) {
                    for (etsl_choice& ch : cat.choices) {
                        unique_sort(ch.if_props);
                        unique_sort(ch.else_props);
                    }
                }
            }
        };
    }

    etsl_file etsl_parse(const std::vector<etsl_token>& tokens)
    {
        etsl_file file;
        detail::etsl_parser(file, tokens);
        return file;
    }
}

#endif
