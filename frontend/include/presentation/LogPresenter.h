#pragma once

#include "presentation/ILogView.h"
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <sys/types.h>

using time_point = std::chrono::system_clock::time_point;

class LogPresenter {
    const std::unique_ptr<ILogView> view_;

public:
    explicit LogPresenter(std::unique_ptr<ILogView> view);

    std::string getViewId();

    void linkSubpage(
            time_point timestamp,
            std::string_view subpage_id,
            uid_t uid,
            std::string_view command
    );

    void addLogEntry(time_point timestamp, std::string_view content, bool is_stdout);

    void setStatus(time_point timestamp, int exit_code);
};
