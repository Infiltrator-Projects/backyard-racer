// SPDX-License-Identifier: GPL-3.0-or-later
#include "pixel_app.hpp"
#include "platform.hpp"

#include <infiltratr/core.h>

#include <exception>
#include <iostream>

namespace backyard_racer {

const InfiltratrProjectInfo& project_info() {
    static const InfiltratrProjectInfo info = {
        sizeof(InfiltratrProjectInfo), INFILTRATR_PROJECT_INFO_ABI,
        "Backyard Racer", "backyard-racer", "au.com.infiltrator.backyard-racer",
        "0.6.0-dev", "Infiltrator-Projects/backyard-racer", "development",
        "Shannon Smith", "https://github.com/Infiltrator-Projects/backyard-racer",
        "GPL-3.0-or-later", "Street-rod garage and racing game", "backyard-racer",
        "Copyright (c) 2000-2026 Shannon Smith"
    };
    return info;
}

} // namespace backyard_racer

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
