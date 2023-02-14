#include "html/HtmlLogView.h"
#include "html/HtmlMainView.h"
#include "html/HtmlViewFactory.h"
#include <chrono>
#include <fmt/chrono.h>
#include <memory>
#include <utility>

using time_point = std::chrono::system_clock::time_point;

HtmlViewFactory::HtmlViewFactory(
        std::filesystem::path directory,
        std::string_view index_file_basename = "index",
        std::string_view styles_file_basename = "styles",
        bool generate_parent_links = true
) : directory_(std::move(directory)),
    index_file_basename_(index_file_basename),
    styles_file_basename_(styles_file_basename),
    generate_parent_links_(generate_parent_links) {}

std::unique_ptr<IMainView> HtmlViewFactory::createMainView() {
    return std::make_unique<HtmlMainView>(directory_, index_file_basename_, styles_file_basename_);
}

static std::string createPageId(time_point timestamp, pid_t pid) {
    using namespace std::chrono;
    auto duration = timestamp.time_since_epoch();
    uint64_t nanos = (duration_cast<nanoseconds>(duration) % seconds(1)).count();
    return fmt::format("{:%Y-%m-%d_%H-%M-%S}.{}_{}", timestamp, nanos, pid);
}

std::unique_ptr<ILogView> HtmlViewFactory::createLogView(
        std::string_view parent_page_id,
        pid_t pid,
        uid_t uid,
        std::string_view command,
        time_point timestamp
) {
    return std::make_unique<HtmlLogView>(
            directory_,
            styles_file_basename_,
            generate_parent_links_,
            createPageId(timestamp, pid),
            parent_page_id,
            uid,
            command,
            timestamp
    );
}

HtmlViewFactory::~HtmlViewFactory() = default;
