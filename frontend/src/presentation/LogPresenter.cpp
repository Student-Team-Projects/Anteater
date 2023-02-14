#include "presentation/LogPresenter.h"
#include "presentation/ILogView.h"
#include <utility>

using time_point = std::chrono::system_clock::time_point;

LogPresenter::LogPresenter(std::unique_ptr<ILogView> view) : view_(std::move(view)) {}

std::string LogPresenter::getViewId() {
    return view_->getId();
}

void LogPresenter::linkSubpage(
        time_point timestamp,
        std::string_view subpage_id, uid_t uid,
        std::string_view command
) {
    view_->linkSubpage(timestamp, subpage_id, uid, command);
}

void LogPresenter::addLogEntry(time_point timestamp, std::string_view content, bool is_stdout) {
    view_->addLogEntry(timestamp, content, is_stdout);
}

void LogPresenter::setStatus(time_point timestamp, int exit_code) {
    view_->setStatus(timestamp, exit_code);
}
