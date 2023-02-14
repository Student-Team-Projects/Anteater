#pragma once

#include "presentation/ILogView.h"
#include "presentation/IMainView.h"
#include "presentation/LogPresenter.h"
#include <chrono>
#include <memory>
#include <string_view>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

using time_point = std::chrono::system_clock::time_point;

class MainPresenter {
public:
    class Factory {
    public:
        virtual std::unique_ptr<LogPresenter> createLogPresenter(
                time_point timestamp,
                std::string_view parent_page_id,
                pid_t pid,
                uid_t uid,
                std::string_view command
        ) = 0;

        virtual ~Factory() = default;
    };

private:
    const std::unique_ptr<IMainView> view_;
    const std::shared_ptr<MainPresenter::Factory> factory_;
    std::unordered_map<pid_t, std::shared_ptr<LogPresenter>> current_subpages_;
    std::unordered_map<pid_t, std::vector<std::shared_ptr<LogPresenter>>> created_subpages_;

public:
    MainPresenter(std::unique_ptr<IMainView> view, std::shared_ptr<MainPresenter::Factory> factory);

    std::string getViewId();

    void addCapture(time_point timestamp, pid_t pid, uid_t uid, std::string_view command);

    void addForkEvent([[maybe_unused]] time_point timestamp, pid_t pid, pid_t child_pid);

    void addExecEvent(time_point timestamp, pid_t pid, uid_t uid, std::string_view command);

    void addWriteEvent(time_point timestamp, pid_t pid, std::string_view content, bool is_stdout);

    void addExitEvent(time_point timestamp, pid_t pid, int exit_code);
};
