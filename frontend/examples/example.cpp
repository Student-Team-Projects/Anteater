#include "html/HtmlViewFactory.h"
#include "presentation/PresenterFactory.h"
#include <chrono>
#include <filesystem>
#include <fmt/core.h>
#include <memory>
#include <sys/types.h>
#include <utility>

// (pid=1000, uid=0) "./sample_program arg1 arg2"
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

void runExample(const std::filesystem::path &directory) {
    auto view_factory = std::make_unique<HtmlViewFactory>(directory, "index", "styles", true);
    auto presenter_factory = std::make_unique<PresenterFactory>(std::move(view_factory));
    auto main_presenter = presenter_factory->createMainPresenter();

    pid_t root_pid = 1000;
    uid_t root_uid = 0;
    pid_t child_pid = 2000;
    uid_t child_uid = 1;
    auto timestamp = std::chrono::system_clock::now();
    auto second = std::chrono::seconds(1);

    main_presenter->addCapture(timestamp, root_pid, root_uid, "./sample_program arg1 arg2");
    main_presenter->addWriteEvent(timestamp += second, root_pid, "Hello, stdout!", true);
    main_presenter->addWriteEvent(timestamp += second, root_pid, "Hello, stderr!", false);
    main_presenter->addForkEvent(timestamp += second, root_pid, child_pid);
    main_presenter->addExecEvent(timestamp += second, child_pid, child_uid, "/usr/bin/ls /path");
    main_presenter->addWriteEvent(timestamp += second, child_pid, "/usr/bin/ls: cannot access '/path'", false);
    main_presenter->addExitEvent(timestamp += second, child_pid, 1);
    main_presenter->addExecEvent(timestamp += second, root_pid, child_uid, "/usr/bin/echo hello");
    main_presenter->addWriteEvent(timestamp += second, root_pid, "hello", true);
    main_presenter->addExitEvent(timestamp += second, root_pid, 0);
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
    runExample(output_dir);
    return 0;
}
