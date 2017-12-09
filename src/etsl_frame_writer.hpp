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

#ifndef ETSL_OUTPUT_HPP
#define ETSL_OUTPUT_HPP

#include <fstream>
#include <iomanip>

#include "etsl_file.hpp"

namespace etsl {
    namespace details {
        class etsl_frame_writer {
        private:
            std::ostream& os_;
            const etsl_file& file_;
            int frame_num_ = 0;
            size_t cat_name_maxlen_;

            struct category_choice_state {
                int selected;
                const std::vector<std::string>* props = nullptr;
            };

            void write_frame_heading()
            {
                os_ << "\nTest Case ";
                os_ << std::left << std::setw(3) << (frame_num_ + 1);
                os_ << "\t\t";
                ++frame_num_;
            }

            void write_single_frame(const etsl_category& category,
                                    const etsl_choice& choice,
                                    const std::string& single_str,
                                    const std::string& if_or_else)
            {
                if (single_str.empty()) {
                    return;
                }

                write_frame_heading();
                os_ << "<" << single_str << ">";
                if (!if_or_else.empty()) {
                    os_ << "  (follows [" << if_or_else << "])";
                }
                os_ << "\n";

                os_ << "   " << category.name << " :  " << choice.name
                    << "\n\n";
            }

            void write_single_frames()
            {
                for (const auto& cat : file_.categories) {
                    for (const auto& ch : cat.choices) {
                        write_single_frame(cat, ch, ch.single_str, "");
                        write_single_frame(cat, ch, ch.if_single_str, "if");
                        write_single_frame(cat, ch, ch.else_single_str, "else");
                    }
                }
            }

            void
            write_normal_frame(std::vector<category_choice_state>& state_stack)
            {
                int level = state_stack.size();

                write_frame_heading();
                os_ << "(Key = ";
                for (int i = 0; i < level; ++i) {
                    os_ << (state_stack[i].selected + 1) << ".";
                }
                os_ << ")\n";

                for (int i = 0; i < level; ++i) {
                    const etsl_category& cat = file_.categories[i];
                    int sel = state_stack[i].selected;

                    os_ << "   " << std::setw(cat_name_maxlen_);
                    os_ << cat.name;
                    os_ << " :  ";
                    os_ << (sel == -1 ? "<n/a>" : cat.choices[sel].name);
                    os_ << "\n";
                }
                os_ << "\n";
            }

            void visit_category(std::vector<category_choice_state>& state_stack)
            {
                size_t level = state_stack.size();
                if (level >= file_.categories.size()) {
                    write_normal_frame(state_stack);
                    return;
                }

                state_stack.emplace_back();

                bool selected = false;

                const auto& cat = file_.categories[level];
                for (size_t i = 0; i < cat.choices.size(); ++i) {
                    const auto& ch = cat.choices[i];

                    auto prop_map = [&](const std::string& s) {
                        for (int j = level - 1; j >= 0; --j) {
                            const auto* p = state_stack[j].props;
                            if (p != nullptr
                                && std::binary_search(begin(*p), end(*p), s)) {
                                return true;
                            }
                        }
                        return false;
                    };
                    if (!ch.has_if) {
                        if (ch.single_str.empty()) {
                            state_stack.back().selected = i;
                            state_stack.back().props = &ch.if_props;
                            visit_category(state_stack);
                            selected = true;
                        }
                    }
                    else if (ch.cond(prop_map)) {
                        if (ch.single_str.empty() && ch.if_single_str.empty()) {
                            state_stack.back().selected = i;
                            state_stack.back().props = &ch.if_props;
                            visit_category(state_stack);
                            selected = true;
                        }
                    }
                    else if (ch.has_else) {
                        if (ch.single_str.empty()
                            && ch.else_single_str.empty()) {
                            state_stack.back().selected = i;
                            state_stack.back().props = &ch.else_props;
                            visit_category(state_stack);
                            selected = true;
                        }
                    }

                    if (selected && cat.mutually_exclusive) {
                        break;
                    }
                }

                // If none is selected for this category, we need to select N/A.
                if (!selected) {
                    state_stack.back().selected = -1;
                    state_stack.back().props = nullptr;
                    visit_category(state_stack);
                }

                state_stack.pop_back();
            }

            void write_normal_frames()
            {
                std::vector<category_choice_state> state_stack;
                visit_category(state_stack);
            }

        public:
            etsl_frame_writer(std::ostream& os, const etsl_file& file)
                    : os_(os), file_(file)
            {
                // Compute the maximum length of the category names.
                cat_name_maxlen_ = 0;
                for (const etsl_category& cat : file_.categories) {
                    if (cat_name_maxlen_ < cat.name.size()) {
                        cat_name_maxlen_ = cat.name.size();
                    }
                }
            }

            void write()
            {
                write_single_frames();
                write_normal_frames();
            }
        };
    }

    void write_tsl_frames(std::ostream& os, const etsl_file& file)
    {
        details::etsl_frame_writer writer(os, file);
        writer.write();
    }
}

#endif
