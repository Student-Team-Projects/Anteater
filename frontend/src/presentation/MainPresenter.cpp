#include "presentation/IMainView.h"
#include "presentation/MainPresenter.h"
#include <utility>

using time_point = std::chrono::system_clock::time_point;

MainPresenter::MainPresenter(
        std::unique_ptr<IMainView> view,
        std::shared_ptr<MainPresenter::Factory> factory
) : view_(std::move(view)),
    factory_(std::move(factory)) {}

std::string MainPresenter::getViewId() {
    return view_->getId();
}

void MainPresenter::addCapture(time_point timestamp, pid_t pid, uid_t uid, std::string_view command) {
    auto root = factory_->createLogPresenter(timestamp, this->getViewId(), pid, uid, command);
    view_->addCapture(root->getViewId(), uid, command, timestamp);
    current_subpages_[pid] = std::move(root);
    created_subpages_[pid].assign({current_subpages_[pid]});
}

void MainPresenter::addExecEvent(time_point timestamp, pid_t pid, uid_t uid, std::string_view command) {
    auto parent = current_subpages_.at(pid);
    auto child = factory_->createLogPresenter(timestamp, parent->getViewId(), pid, uid, command);
    parent->linkSubpage(timestamp, child->getViewId(), uid, command);
    current_subpages_[pid] = std::move(child);
    created_subpages_[pid].push_back(current_subpages_[pid]);
}

void MainPresenter::addForkEvent([[maybe_unused]] time_point timestamp, pid_t pid, pid_t child_pid) {
    current_subpages_[child_pid] = current_subpages_.at(pid);
}

void MainPresenter::addWriteEvent(
        time_point timestamp,
        pid_t pid,
        std::string_view content,
        bool is_stdout
) {
    current_subpages_.at(pid)->addLogEntry(timestamp, content, is_stdout);
}

void MainPresenter::addExitEvent(time_point timestamp, pid_t pid, int exit_code) {
    for (const auto &subpage: created_subpages_[pid]) {
        subpage->setStatus(timestamp, exit_code);
    }
    created_subpages_.erase(pid);
    current_subpages_.erase(pid);
}
