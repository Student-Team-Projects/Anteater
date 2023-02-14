#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <sys/types.h>

using time_point = std::chrono::system_clock::time_point;

class MainPresenter;

class IMainView {
public:
    virtual std::string getId() = 0;

    virtual void addCapture(
            std::string_view root_page_id,
            uid_t uid,
            std::string_view command,
            time_point timestamp
    ) = 0;

    virtual ~IMainView() = default;
};
