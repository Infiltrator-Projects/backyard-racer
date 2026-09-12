// SPDX-License-Identifier: GPL-3.0-or-later
#include "pixel_app.hpp"
#include "platform.hpp"

#include <infiltratr/core.h>

#include "generated_assets.h"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    const InfiltratrProjectInfo& info = backyard_racer::project_info();
    if (!infiltratr_project_info_is_valid(&info)) {
        std::cerr << "Backyard Racer project metadata is invalid\n";
        return 1;
    }
    if (argc == 2 && infiltratr_string_equal(argv[1], "--version")) {
        std::cout << info.program_name << ' ' << info.version << '\n';
        return 0;
    }
    if (argc == 2 && infiltratr_string_equal(argv[1], "--project-info")) {
        return infiltratr_project_info_print(stdout, &info) == 0 ? 0 : 1;
    }

    try {
        backyard_racer::PixelApp app(1280, 720);
        return backyard_racer::platform::run(app);
    } catch (const std::exception& error) {
        std::cerr << info.program_name << " failed to start: " << error.what() << '\n';
        return 1;
    }
}
