#pragma once

#include "presentation/ILogView.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <sys/types.h>

using time_point = std::chrono::system_clock::time_point;

class HtmlLogView : public ILogView {
    const std::string page_id_;
    std::ofstream html_file_;
    std::ofstream js_file_;

public:
    HtmlLogView(
            const std::filesystem::path &directory,
            std::string_view styles_file_basename,
            bool generate_parent_links,
            std::string_view page_id,
            std::string_view parent_page_id,
            uid_t uid,
            std::string_view command,
            time_point timestamp
    );

    std::string getId() override;

    void addLogEntry(time_point timestamp, std::string_view content, bool is_stdout) override;

    void linkSubpage(
            time_point timestamp,
            std::string_view subpage_id,
            uid_t uid,
            std::string_view command
    ) override;

    void setStatus(time_point timestamp, int exit_code) override;

    ~HtmlLogView() override;
};
