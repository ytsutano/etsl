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

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>

#include "etsl_parser.hpp"
#include "etsl_frame_writer.hpp"

struct program_configuration {
    bool count_only = false;
    std::string input_filename = "";
    std::string output_filename = "";
};

void print_manpage()
{
    std::cout << "(Manpage)\n";
}

program_configuration parse_arguments(int argc, char** argv)
{
    program_configuration config;
    bool use_stdout = false;

    if (argc < 2) {
        std::cerr << "usage: etsl [ --manpage ] [ -cs ] input_file [ -o "
                     "output_file ]\n";
        std::exit(1);
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--manpage") {
            print_manpage();
            std::exit(0);
        }

        if (!arg.empty() && arg[0] == '-') {
            for (char c : arg) {
                switch (c) {
                case 'c':
                    config.count_only = true;
                    break;
                case 's':
                    use_stdout = true;
                    break;
                case 'o':
                    ++i;
                    if (i >= argc) {
                        throw std::runtime_error("invalid arguments");
                    }
                    config.output_filename = argv[i];
                    break;
                }
            }
        }
        else {
            config.input_filename = arg;
        }
    }

    if (config.input_filename == "") {
        throw std::runtime_error("missing input filename");
    }

    if (!use_stdout && config.output_filename.empty()) {
        config.output_filename = config.input_filename + ".tsl";
    }

    return config;
}

int main(int argc, char** argv)
{
    try {
        auto config = parse_arguments(argc, argv);

        try {
            // Read TSL file.
            std::ifstream ifs(config.input_filename);
            auto tokens = etsl::etsl_tokenize(ifs);
            auto file = etsl::etsl_parse(tokens);

            // Write frames.
            if (!config.output_filename.empty()) {
                std::ofstream ofs(config.output_filename);
                etsl::write_tsl_frames(ofs, file);
            }
            else {
                etsl::write_tsl_frames(std::cout, file);
            }
        }
        catch (etsl::etsl_syntax_error& e) {
            std::cerr << config.input_filename << ":";
            std::cerr << e.line_num << ":";
            std::cerr << e.col_num << ": ";
            std::cerr << e.what() << "\n";
            std::exit(1);
        }
    }
    catch (std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::exit(1);
    }
}
