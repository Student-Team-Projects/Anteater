#pragma once

#include "presentation/ILogView.h"
#include "presentation/IMainView.h"
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <sys/types.h>

using time_point = std::chrono::system_clock::time_point;

class IViewFactory {
public:
    virtual std::unique_ptr<IMainView> createMainView() = 0;

    virtual std::unique_ptr<ILogView> createLogView(
            std::string_view parent_page_id,
            pid_t pid,
            uid_t uid,
            std::string_view command,
            time_point timestamp
    ) = 0;

    virtual ~IViewFactory() = default;
};
