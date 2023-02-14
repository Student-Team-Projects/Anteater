#include "presentation/IViewFactory.h"
#include "presentation/LogPresenter.h"
#include "presentation/MainPresenter.h"
#include "presentation/PresenterFactory.h"
#include <memory>
#include <utility>

using time_point = std::chrono::system_clock::time_point;

PresenterFactory::PresenterFactory(
        std::shared_ptr<IViewFactory> view_factory
) : view_factory_(std::move(view_factory)) {}

std::unique_ptr<MainPresenter> PresenterFactory::createMainPresenter() {
    auto view = view_factory_->createMainView();
    auto this_copy = std::make_shared<PresenterFactory>(view_factory_);
    return std::make_unique<MainPresenter>(std::move(view), this_copy);
}

std::unique_ptr<LogPresenter> PresenterFactory::createLogPresenter(
        time_point timestamp,
        std::string_view parent_page_id,
        pid_t pid,
        uid_t uid,
        std::string_view command
) {
    auto view = view_factory_->createLogView(parent_page_id, pid, uid, command, timestamp);
    return std::make_unique<LogPresenter>(std::move(view));
}

PresenterFactory::~PresenterFactory() = default;
