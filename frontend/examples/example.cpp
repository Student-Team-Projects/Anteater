#include "html/HtmlViewFactory.h"
#include "presentation/PresenterFactory.h"
#include <chrono>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <utility>

const std::filesystem::path HTOP_OUTPUT_FILENAME = "examples/resources/htop.in";
const std::filesystem::path RAINBOW_FILENAME = "examples/resources/rainbow.in";
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

void addHtopCapture(const std::unique_ptr<MainPresenter> &main_presenter) {
    auto timestamp = std::chrono::system_clock::now();
    pid_t pid = 1000;
    uid_t uid = 0;
    std::string htop_output = readFileToString(HTOP_OUTPUT_FILENAME);

    main_presenter->addCapture(timestamp, pid, uid, "./htop_example");
    main_presenter->addWriteEvent(timestamp += SECOND, pid, htop_output, true);
    main_presenter->addExitEvent(timestamp += SECOND, pid, 0);
}

void addRainbowCapture(const std::unique_ptr<MainPresenter> &main_presenter) {
    auto timestamp = std::chrono::system_clock::now();
    pid_t pid = 1000;
    uid_t uid = 0;
    std::string rainbow = readFileToString(RAINBOW_FILENAME);

    main_presenter->addCapture(timestamp, pid, uid, "./rainbow_example");
    main_presenter->addWriteEvent(timestamp += SECOND, pid, rainbow, true);
    main_presenter->addExitEvent(timestamp += SECOND, pid, 0);
}

void runExamples(const std::filesystem::path &directory) {
    auto view_factory = std::make_unique<HtmlViewFactory>(directory, "index", "styles", true);
    auto presenter_factory = std::make_unique<PresenterFactory>(std::move(view_factory));
    auto main_presenter = presenter_factory->createMainPresenter();

    addForkCapture(main_presenter);
    addHtopCapture(main_presenter);
    addRainbowCapture(main_presenter);
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
