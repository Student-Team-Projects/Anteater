#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <sys/types.h>

using time_point = std::chrono::system_clock::time_point;

class LogPresenter;

class ILogView {
public:
    virtual std::string getId() = 0;

    virtual void addLogEntry(time_point timestamp, std::string_view content, bool is_stdout) = 0;

    virtual void linkSubpage(
            time_point timestamp,
            std::string_view subpage_id,
            uid_t uid,
            std::string_view command
    ) = 0;

    virtual void setStatus(time_point timestamp, int exit_code) = 0;

    virtual ~ILogView() = default;
};
