#include "html/HtmlLogView.h"
#include "html/utils.h"
#include "html/AnsiParser.h"
#include <fmt/chrono.h>
#include <fmt/ostream.h>
#include <string>

using time_point = std::chrono::system_clock::time_point;

constexpr char HTML_HEADER_FMT[] = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <link rel="stylesheet" type="text/css" href="{styles}.css">
  <title>{title}</title>
</head>
<body>
  {parent_link}
  <div id="info">
    <h2>Info</h2>
    <table>
      <tr><td>User:</td><td>{user}</td></tr>
      <tr><td>Command:</td><td>{command}</td></tr>
      <tr><td>Start:</td><td class="timestamp">{start_time}</td></tr>
      <tr>
        <td>Status:</td>
        <td id="{page_id}_status" class="wait">
          <span id="{page_id}_status_code">exit ?</span>
        </td>
      </tr>
      <script src="{page_id}.js"></script>
    </table>
  </div>
  <div id="logs">
    <h2>Logs</h2>
    <table>
)";

constexpr char LOG_FMT[] = R"(
      <tr>
        <td class="timestamp">{timestamp}</td>
        <td class="log write {stream_class}">{content}</td>
      </tr>
)";

constexpr char SUBPAGE_LINK_FMT[] = R"(
      <tr>
        <td class="timestamp">{timestamp}</td>
        <td id="{page_id}_status" class="log exec wait">
          <span id="{page_id}_status_code">exit ?</span>; {user}; <a href="{page_id}.html">{command}</a>
        </td>
      </tr>
      <script src="{page_id}.js"></script>
)";

// Statements are enclosed in braces to avoid variable redefinition errors.
constexpr char JS_FMT[] = R"(
{{
  let element;
  element = document.getElementById('{page_id}_status_code');
  element.textContent = 'exit {exit_code}';
  element = document.getElementById('{page_id}_status');
  element.classList.remove('wait');
  element.classList.add('{status_class}');
  document.currentScript.remove();
}}
)";

HtmlLogView::HtmlLogView(
        const std::filesystem::path &directory,
        std::string_view styles_file_basename,
        bool generate_parent_links,
        std::string_view page_id,
        std::string_view parent_page_id,
        uid_t uid,
        std::string_view command,
        time_point timestamp
) : page_id_(page_id),
    html_file_(directory / (page_id_ + ".html")),
    js_file_(directory / (page_id_ + ".js")) {
    std::string parent_link;
    if (generate_parent_links) {
        parent_link = fmt::format(R"(<a id="parent-link" href="{}.html">Go to parent</a>)", parent_page_id);
    }
    fmt::print(html_file_, HTML_HEADER_FMT,
               fmt::arg("page_id", page_id_),
               fmt::arg("title", command),
               fmt::arg("styles", styles_file_basename),
               fmt::arg("parent_link", parent_link),
               fmt::arg("user", getUserInfoString(uid)),
               fmt::arg("command", command),
               fmt::arg("start_time", timestamp)
    );
    html_file_ << std::flush;
}

std::string HtmlLogView::getId() {
    return page_id_;
}

void HtmlLogView::addLogEntry(time_point timestamp, std::string_view content, bool is_stdout) {
    fmt::print(html_file_, LOG_FMT,
               fmt::arg("timestamp", timestamp),
               fmt::arg("content", convert(content)),
               fmt::arg("stream_class", is_stdout ? "stdout" : "stderr")
    );
    html_file_ << std::flush;
}

void HtmlLogView::linkSubpage(time_point timestamp, std::string_view subpage_id, uid_t uid, std::string_view command) {
    fmt::print(html_file_, SUBPAGE_LINK_FMT,
               fmt::arg("timestamp", timestamp),
               fmt::arg("page_id", subpage_id),
               fmt::arg("user", getUserInfoString(uid)),
               fmt::arg("command", command)
    );
    html_file_ << std::flush;
}

void HtmlLogView::setStatus(time_point timestamp, int exit_code) {
    fmt::print(js_file_, JS_FMT,
               fmt::arg("page_id", page_id_),
               fmt::arg("exit_code", exit_code),
               fmt::arg("status_class", exit_code == 0 ? "ok" : "error")
    );
    html_file_ << std::flush;
}

HtmlLogView::~HtmlLogView() {
    html_file_.close();
    js_file_.close();
}
