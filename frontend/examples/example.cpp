#include "html/HtmlViewFactory.h"
#include "presentation/PresenterFactory.h"
#include <chrono>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <utility>

const std::filesystem::path HTOP_OUTPUT_FILENAME = "examples/resources/htop.in";
const auto SECOND = std::chrono::seconds(1);

std::string readFileToString(const std::filesystem::path &file) {
    std::ifstream inputFile(file);
    if (!inputFile) {
        fmt::print(stderr, "Failed to open {}, returning empty string", file.string());
        return "";
    }
    std::stringstream buffer;
    buffer << inputFile.rdbuf();
    return buffer.str();
}

// (pid=1000, uid=0) "./fork_example arg1 arg2"
//  |
// WRITE stdout "Hello, stdout!"
//  |
// WRITE stderr "Hello, stderr!"
//  |
// FORK
//  | (pid=2000, uid=1)
//  |  |
//  | EXEC "/usr/bin/ls /path"
//  |  |
//  | WRITE stderr "/usr/bin/ls: cannot access '/path'"
//  |  |
//  | EXIT 1
//  |
// EXEC "/usr/bin/echo hello"
//  |
// WRITE stdout "hello"
//  |
// EXIT 0
void addForkCapture(const std::unique_ptr<MainPresenter> &main_presenter) {
    auto timestamp = std::chrono::system_clock::now();
    pid_t root_pid = 1000;
    uid_t root_uid = 0;
    pid_t child_pid = 2000;
    uid_t child_uid = 1;

    main_presenter->addCapture(timestamp, root_pid, root_uid, "./fork_example arg1 arg2");
    main_presenter->addWriteEvent(timestamp += SECOND, root_pid, "Hello, stdout!", true);
    main_presenter->addWriteEvent(timestamp += SECOND, root_pid, "Hello, stderr!", false);
    main_presenter->addForkEvent(timestamp += SECOND, root_pid, child_pid);
    main_presenter->addExecEvent(timestamp += SECOND, child_pid, child_uid, "/usr/bin/ls /path");
    main_presenter->addWriteEvent(timestamp += SECOND, child_pid, "/usr/bin/ls: cannot access '/path'", false);
    main_presenter->addExitEvent(timestamp += SECOND, child_pid, 1);
    main_presenter->addExecEvent(timestamp += SECOND, root_pid, child_uid, "/usr/bin/echo hello");
    main_presenter->addWriteEvent(timestamp += SECOND, root_pid, "hello", true);
    main_presenter->addExitEvent(timestamp += SECOND, root_pid, 0);
}

const std::string RESET = "\033[0m";

template<typename OutputIt>
void generateTextStylesExample(OutputIt out) {
    fmt::format_to(out, "\033[0mNormal{}\n", RESET);
    fmt::format_to(out, "\033[1mBold{}\n", RESET);
    fmt::format_to(out, "\033[2mFaint{}\n", RESET);
    fmt::format_to(out, "\033[3mItalic{}\n", RESET);
    fmt::format_to(out, "\033[4mUnderline{}\n", RESET);
    fmt::format_to(out, "\033[5mBlink{}\n", RESET);
    fmt::format_to(out, "\033[7mInverse{}\n", RESET);
    fmt::format_to(out, "\033[8mHidden{}\n", RESET);
    fmt::format_to(out, "\033[9mStrikethrough{}\n", RESET);
}

template<typename OutputIt>
void generate8BitColorsExample(OutputIt out, bool foreground) {
    const int code = foreground ? 38 : 48;
    auto printColor = [&out, code](int c) {
        fmt::format_to(
                out,
                "\033[{code};5;{c}m {c:03} {reset}",
                fmt::arg("code", code),
                fmt::arg("c", c),
                fmt::arg("reset", RESET)
        );
    };

    fmt::format_to(out, "Standard:\n");
    for (int c = 0; c < 8; ++c) {
        printColor(c);
    }
    fmt::format_to(out, "\n");

    fmt::format_to(out, "High-intensity:\n");
    for (int c = 8; c < 16; ++c) {
        printColor(c);
    }
    fmt::format_to(out, "\n");

    fmt::format_to(out, "Palette:\n");
    for (int r = 0; r < 6; ++r) {
        for (int g = 0; g < 6; ++g) {
            for (int b = 0; b < 6; ++b) {
                int c = 16 + 36 * r + 6 * g + b;
                printColor(c);
            }
        }
        fmt::format_to(out, "\n");
    }

    fmt::format_to(out, "Grayscale:\n");
    for (int c = 232; c < 256; ++c) {
        printColor(c);
    }
    fmt::format_to(out, "\n");
}

// To generate a representative set of 24-bit colors, we increment each color
// channel (red, green, and blue) by 51 to obtain a total of 6 values (0, 51,
// 102, 153, 204, and 255) for each channel. This results in a total of 216
// evenly distributed colors across the entire 24-bit color space.
template<typename OutputIt>
void generate24BitColorsExample(OutputIt out, bool foreground) {
    const int code = foreground ? 38 : 48;
    auto printColor = [&out, code](int r, int g, int b) {
        fmt::format_to(
                out,
                "\033[{code};2;{r};{g};{b}m #{r:02x}{g:02x}{b:02x} {reset}",
                fmt::arg("code", code),
                fmt::arg("r", r),
                fmt::arg("g", g),
                fmt::arg("b", b),
                fmt::arg("reset", RESET)
        );
    };

    for (int r = 0; r < 256; r += 51) {
        for (int g = 0; g < 256; g += 51) {
            for (int b = 0; b < 256; b += 51) {
                printColor(r, g, b);
            }
            fmt::format_to(out, "\n");
        }
    }
}

void addTextStylesCapture(const std::unique_ptr<MainPresenter> &main_presenter) {
    auto timestamp = std::chrono::system_clock::now();
    pid_t pid = 1000;
    uid_t uid = 0;

    main_presenter->addCapture(timestamp, pid, uid, "./ansiTextStyles");
    {
        fmt::memory_buffer buf;
        generateTextStylesExample(std::back_inserter(buf));
        main_presenter->addWriteEvent(timestamp += SECOND, pid, fmt::to_string(buf), true);
    }
    main_presenter->addExitEvent(timestamp += SECOND, pid, 0);
}

void addColorsCapture(const std::unique_ptr<MainPresenter> &main_presenter, bool foreground) {
    auto timestamp = std::chrono::system_clock::now();
    pid_t pid = 1000;
    uid_t uid = 0;

    main_presenter->addCapture(timestamp, pid, uid, fmt::format("./ansiColors fg={}", foreground));
    {
        fmt::memory_buffer buf;
        generate8BitColorsExample(std::back_inserter(buf), foreground);
        main_presenter->addWriteEvent(timestamp += SECOND, pid, fmt::to_string(buf), true);
    }
    {
        fmt::memory_buffer buf;
        generate24BitColorsExample(std::back_inserter(buf), foreground);
        main_presenter->addWriteEvent(timestamp += SECOND, pid, fmt::to_string(buf), true);
    }
    main_presenter->addExitEvent(timestamp += SECOND, pid, 0);
}

void addHtopCapture(const std::unique_ptr<MainPresenter> &main_presenter) {
    auto timestamp = std::chrono::system_clock::now();
    pid_t pid = 1000;
    uid_t uid = 0;
    std::string htop_output = readFileToString(HTOP_OUTPUT_FILENAME);

    main_presenter->addCapture(timestamp, pid, uid, "./htop_example");
    main_presenter->addWriteEvent(timestamp += SECOND, pid, htop_output, true);
    main_presenter->addExitEvent(timestamp += SECOND, pid, 0);
}

void runExamples(const std::filesystem::path &directory) {
    auto view_factory = std::make_unique<HtmlViewFactory>(directory, "index", "styles", true);
    auto presenter_factory = std::make_unique<PresenterFactory>(std::move(view_factory));
    auto main_presenter = presenter_factory->createMainPresenter();

    addForkCapture(main_presenter);
    addTextStylesCapture(main_presenter);
    addColorsCapture(main_presenter, true);
    addColorsCapture(main_presenter, false);
    addHtopCapture(main_presenter);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fmt::print(stderr, "usage: {} output_dir\n", argv[0]);
        return 1;
    }
    const char *output_dir = argv[1];
    if (!std::filesystem::is_directory(output_dir)) {
        fmt::print(stderr, "{} is not a valid directory\n", output_dir);
        return 1;
    }
    runExamples(output_dir);
    return 0;
}
