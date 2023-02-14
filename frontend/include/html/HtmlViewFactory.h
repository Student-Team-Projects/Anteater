#pragma once

#include "presentation/IMainView.h"
#include "presentation/IViewFactory.h"
#include <chrono>
#include <filesystem>
#include <memory>
#include <string_view>
#include <sys/types.h>

using time_point = std::chrono::system_clock::time_point;

class HtmlViewFactory : public IViewFactory {
    const std::filesystem::path directory_;
    const std::string index_file_basename_;
    const std::string styles_file_basename_;
    const bool generate_parent_links_;

public:
    HtmlViewFactory(
            std::filesystem::path directory,
            std::string_view index_file_basename,
            std::string_view styles_file_basename,
            bool generate_parent_links
    );

    std::unique_ptr<IMainView> createMainView() override;

    std::unique_ptr<ILogView> createLogView(
            std::string_view parent_page_id,
            pid_t pid,
            uid_t uid,
            std::string_view command,
            time_point timestamp
    ) override;

    ~HtmlViewFactory() override;
};
