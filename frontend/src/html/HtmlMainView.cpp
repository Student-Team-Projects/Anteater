#include "html/HtmlMainView.h"
#include "html/utils.h"
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/ostream.h>
#include <fstream>
#include <string>

using time_point = std::chrono::system_clock::time_point;

constexpr char HTML_HEADER[] = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <link rel="stylesheet" type="text/css" href="{styles}.css">
  <title>Debugger</title>
</head>
<body>
  <div id="logs">
    <h2>Captures</h2>
    <table>
)";

constexpr char CAPTURE_FMT[] = R"(
      <tr>
        <td class="timestamp">{timestamp}</td>
        <td id="{page_id}_status" class="log exec wait">
          <span id="{page_id}_status_code">exit ?</span>; {user}; <a href="{page_id}.html">{command}</a>
        </td>
      </tr>
      <script src="{page_id}.js"></script>
)";

constexpr char STYLES[] = R"(
:root {
  --color-background: #1a232f;
  --color-text-primary: #dbdee0;
  --color-text-secondary: #a0aab2;
  --color-border: #253240;
  --color-box-shadow: #101925;
  --color-link: #80b4da;
  --color-wait: #d3b066;
  --color-ok: #8fae5f;
  --color-error: #c45b5a;
  --color-stderr: #ecae9b;
}

body {
  font-family: monospace;
  margin: 20px;
  background-color: var(--color-background);
  color: var(--color-text-primary);
}

div {
  margin-top: 20px;
  padding: 20px;
  border: solid 1px var(--color-border);
  border-radius: 10px;
  box-shadow: 0 2px 6px 2px var(--color-box-shadow);
}

h2 {
  margin-top: 0;
  margin-bottom: 12px;
  color: var(--color-text-primary);
}

#parent-link {
  font-weight: bold;
  color: var(--color-link);
}

#info td:first-child {
  font-weight: bold;
}

#logs table {
  border-collapse: collapse;
}

#logs table td {
  padding-left: 10px;
  padding-right: 10px;
}

#logs table td:first-child {
  border-right: solid 1px var(--color-text-primary);
  padding-left: 0;
}

.timestamp {
  overflow: hidden;
  white-space: nowrap;
  color: var(--color-text-secondary);
}

.log.exec {
  overflow: hidden;
  white-space: nowrap;
}

.log.write {
  white-space: pre-wrap;
}

.log.write.stderr {
  color: var(--color-stderr);
}

.wait, .wait * {
  color: var(--color-wait);
}

.ok, .ok * {
  color: var(--color-ok);
}

.error, .error * {
  color: var(--color-error);
}
)";

HtmlMainView::HtmlMainView(
        const std::filesystem::path &directory,
        std::string_view index_file_basename,
        std::string_view styles_file_basename
) : index_file_basename_(index_file_basename) {
    {
        auto path = directory / (std::string(index_file_basename) + ".html");
        bool exists = std::filesystem::exists(path);
        file_.open(path, std::ios::app);
        if (!exists) {
            fmt::print(file_, HTML_HEADER, fmt::arg("styles", styles_file_basename));
        }
    }
    {
        auto path = directory / (std::string(styles_file_basename) + ".css");
        auto exists = std::filesystem::exists(path);
        if (!exists) {
            std::ofstream styles_file(path);
            styles_file << STYLES;
            styles_file.close();
        }
    }
}

std::string HtmlMainView::getId() {
    return index_file_basename_;
}

void HtmlMainView::addCapture(
        std::string_view root_page_id,
        uid_t uid,
        std::string_view command,
        time_point timestamp
) {
    fmt::print(file_, CAPTURE_FMT,
               fmt::arg("page_id", root_page_id),
               fmt::arg("timestamp", timestamp),
               fmt::arg("user", getUserInfoString(uid)),
               fmt::arg("command", command)
    );
}

HtmlMainView::~HtmlMainView() {
    file_.close();
}
