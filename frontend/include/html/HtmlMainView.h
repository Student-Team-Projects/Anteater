#pragma once

#include "presentation/IMainView.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <sys/types.h>

using time_point = std::chrono::system_clock::time_point;

class HtmlMainView : public IMainView {
    const std::string index_file_basename_;
    std::ofstream file_;

public:
    HtmlMainView(
            const std::filesystem::path &directory,
            std::string_view index_file_basename,
            std::string_view styles_file_basename
    );

    std::string getId() override;

    void addCapture(
            std::string_view root_page_id,
            uid_t uid,
            std::string_view command,
            time_point timestamp
    ) override;

    ~HtmlMainView() override;
};
