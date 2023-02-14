#pragma once

#include "presentation/IViewFactory.h"
#include "presentation/MainPresenter.h"
#include <chrono>
#include <memory>
#include <string_view>
#include <sys/types.h>

using time_point = std::chrono::system_clock::time_point;

class PresenterFactory : public MainPresenter::Factory {
    const std::shared_ptr<IViewFactory> view_factory_;

public:
    explicit PresenterFactory(std::shared_ptr<IViewFactory> view_factory);

    std::unique_ptr<MainPresenter> createMainPresenter();

    std::unique_ptr<LogPresenter> createLogPresenter(
            time_point timestamp,
            std::string_view parent_page_id,
            pid_t pid,
            uid_t uid,
            std::string_view command
    ) override;

    ~PresenterFactory() override;
};
