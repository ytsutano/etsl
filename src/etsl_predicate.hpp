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

#ifndef ETSL_PREDICATE_HPP
#define ETSL_PREDICATE_HPP

#include <vector>
#include <memory>

namespace etsl {
    struct etsl_invalid_predicate_error : std::runtime_error {
        using runtime_error::runtime_error;
    };

    class etsl_predicate {
    private:
        struct expression {
            enum { kind_and, kind_or, kind_not, kind_prop } kind;
            std::string prop_name;
            std::unique_ptr<expression> operands[2];
        };

    private:
        std::unique_ptr<expression> expr_;

    private:
        // <expr> ::= <term> ( "||" <term> )*
        //
        // <term> ::= <primary> ( "&&" <primary> )*
        //
        // <primary> ::= "!" <primary>
        //             | "(" <expression> ")"
        //             | <prop>

        template <typename I>
        std::unique_ptr<expression> parse_expr(I& first, I last)
        {
            auto expr = parse_term(first, last);
            if (expr != nullptr) {
                while (first != last && *first == "||") {
                    ++first;
                    auto rhs = parse_term(first, last);
                    if (rhs != nullptr) {
                        auto lhs = std::move(expr);
                        expr = std::unique_ptr<expression>(new expression);
                        expr->kind = expression::kind_or;
                        expr->operands[0] = std::move(lhs);
                        expr->operands[1] = std::move(rhs);
                    }
                    else {
                        throw etsl_invalid_predicate_error(
                                "invalid operator ||");
                    }
                }
            }

            return expr;
        }

        template <typename I>
        std::unique_ptr<expression> parse_term(I& first, I last)
        {
            auto term = parse_primary(first, last);
            if (term != nullptr) {
                while (first != last && *first == "&&") {
                    ++first;
                    auto rhs = parse_primary(first, last);
                    if (rhs != nullptr) {
                        auto lhs = std::move(term);
                        term = std::unique_ptr<expression>(new expression);
                        term->kind = expression::kind_and;
                        term->operands[0] = std::move(lhs);
                        term->operands[1] = std::move(rhs);
                    }
                    else {
                        throw etsl_invalid_predicate_error(
                                "invalid operator &&");
                    }
                }
            }

            return term;
        }

        template <typename I>
        std::unique_ptr<expression> parse_primary(I& first, I last)
        {
            if (first != last) {
                if (*first == "!") {
                    ++first;
                    auto operand = parse_primary(first, last);
                    if (operand != nullptr) {
                        auto expr = std::unique_ptr<expression>(new expression);
                        expr->kind = expression::kind_not;
                        expr->operands[0] = std::move(operand);
                        return expr;
                    }
                    else {
                        throw etsl_invalid_predicate_error(
                                "invalid operator !");
                    }
                }
                else if (*first == "(") {
                    ++first;
                    auto expr = parse_expr(first, last);
                    if (expr != nullptr && first != last && *first == ")") {
                        ++first;
                        return expr;
                    }
                    else {
                        throw etsl_invalid_predicate_error(
                                "invalid parentheses");
                    }
                }
            }

            return parse_prop(first, last);
        }

        template <typename I>
        std::unique_ptr<expression> parse_prop(I& first, I last)
        {
            if (first != last
                && (std::isalnum((*first)[0]) || (*first)[0] == ':')) {
                auto expr = std::unique_ptr<expression>(new expression);
                expr->kind = expression::kind_prop;
                expr->prop_name = *first;
                ++first;
                return expr;
            }

            return nullptr;
        }

        void print_expr(std::ostream& os,
                        const std::unique_ptr<expression>& expr) const
        {
            switch (expr->kind) {
            case expression::kind_prop:
                os << expr->prop_name;
                break;
            case expression::kind_not:
                os << "not(";
                print_expr(os, expr->operands[0]);
                os << ")";
                break;
            case expression::kind_and:
                os << "and(";
                print_expr(os, expr->operands[0]);
                os << ", ";
                print_expr(os, expr->operands[1]);
                os << ")";
                break;
            case expression::kind_or:
                os << "or(";
                print_expr(os, expr->operands[0]);
                os << ", ";
                print_expr(os, expr->operands[1]);
                os << ")";
                break;
            }
        }

        template <typename F>
        bool evaluate(const std::unique_ptr<expression>& expr,
                      const F& prop_map) const
        {
            if (expr == nullptr) {
                return true;
            }

            switch (expr->kind) {
            case expression::kind_prop:
                return prop_map(expr->prop_name);
            case expression::kind_not:
                return !evaluate(expr->operands[0], prop_map);
            case expression::kind_and:
                return evaluate(expr->operands[0], prop_map)
                        && evaluate(expr->operands[1], prop_map);
            case expression::kind_or:
                return evaluate(expr->operands[0], prop_map)
                        || evaluate(expr->operands[1], prop_map);
            }

            return false;
        }

    public:
        friend std::ostream& operator<<(std::ostream& os,
                                        const etsl_predicate& pred)
        {
            pred.print_expr(os, pred.expr_);
            return os;
        }

        template <typename I>
        void parse(I first, I last)
        {
            expr_ = parse_expr(first, last);
            if (expr_ == nullptr) {
                throw etsl_invalid_predicate_error("invalid predicate");
            }
        }

        template <typename F>
        bool operator()(const F& prop_map) const
        {
            return evaluate(expr_, prop_map);
        }
    };
}

#endif
