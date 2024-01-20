#include "structure/html/html_event_formatter.hpp"

#include <regex>

using namespace events;

const std::string CSS = R"(
    <style>
        html { font-family: monospace; }
        body {
            background: #111;
            color: #fff;
        }
        td {
            padding: 2px 10px;
        }
        .event > :first-child {
            border-right: 1px solid #eee;
        }
        a {
            color: lime;
        }
    </style>
)";

const std::string TABLE_BEGIN = "<table><tbody>";
const std::string TABLE_END = "</tbody></table>";

const std::string HORIZONTAL_LINE = "<hr/>";

static auto round_to_millis(std::chrono::system_clock::time_point const& timestamp) {
    return std::chrono::time_point_cast<std::chrono::milliseconds>(timestamp);
}

static void format_last_entry_timestamp(std::ostream& os) {
    os <<
        "<tr>" 
            "<td> last entry timestamp </td>" 
            "<td id='last-entry-timestamp'> ? </td>"
        "</tr>"
        "<script>"
            "document.addEventListener('DOMContentLoaded', function() {"
            "let timestamps = document.getElementsByClassName('timestamp');"
            "if (timestamps.length != 0)"
                "document.getElementById('last-entry-timestamp').textContent = timestamps[timestamps.length - 1].textContent" 
        "});"
        "</script>";
}

static void format_link_to_parent(
    std::ostream& os, 
    std::optional<parent_path_info> const& parent_info
) {
    if(parent_info.has_value()) {
        os << "<tr>" << "<td> parent </td>";
        os << "<td> <a href='./" << parent_info.value().filename << "'>";
        os << parent_info.value().command << " </a>" << "</td></tr>";
    }
}

static void format_exec_info(std::ostream& os, exec_event const& e) {
    os << "<tr>" << "<td> account </td>" << "<td>" << e.user_name << "</td>" << "</tr>";
    os << "<tr>" << "<td> command </td>" << "<td>" << e.command << "</td>" << "</tr>";
    os << "<tr>" << "<td> working directory </td>" << "<td>" << e.working_directory << "</td>" << "</tr>";
    os << "<tr>" << "<td> exec timestamp </td>" << "<td>" << round_to_millis(e.timestamp) << "</td>" << "</tr>";
}

static void format_exit_code(std::ostream& os) {
    os << "<tr> <td> exit code </td> <td id='exit_code'>?</td> </tr>";
}

static void format_return_links(
    std::ostream& os,
    root_path_info const& root_info
) {
    os << "<tr><td> index </td> <td> <a href='" << root_info.index_path.string() << "'> index </a> </td></tr>";
    os << "<tr><td> root command </td> <td> <a href='" << root_info.root_command_path.string() << "'>" << root_info.root_command << "</a> </td></tr>";
}

static void format_page_header(
    std::ostream& os, 
    exec_event const& source_event, 
    root_path_info const& root_info,
    std::optional<parent_path_info> const& parent_info
) {
    os << TABLE_BEGIN;

    format_return_links(os, root_info); 
    format_exec_info(os, source_event);
    format_last_entry_timestamp(os);
    format_exit_code(os);
    format_link_to_parent(os, parent_info);

    os << TABLE_END;
    os << HORIZONTAL_LINE;
}

static void begin_html(std::ostream& os) {
    os << "<!DOCTYPE HTML>"
        << "<html lang='pl-PL'>"

        << "<head>"
        << "<meta charset='UTF-8'/>"
        << CSS
        << "</head>"

        << "<body>";
}


static void begin_event_table(std::ostream& os) { os << "<table><tbody>"; }

void html_event_formatter::begin_index(std::ostream& os) const {
    begin_html(os);
    os << "<h2> index </h2>";
    os << "<hr/>";
    os << TABLE_BEGIN;
}
void html_event_formatter::format_index_link(
    std::ostream& os,
    std::chrono::system_clock::time_point timestamp,
    std::string const& href, 
    std::string const& displayed_text
) const {
    os << "<tr>"
        << "<td>" << round_to_millis(timestamp) << "</td>"
        << "<td> <a href='" << href << "'>" << displayed_text << "</a> </td>"
        << "</tr>";
}

void html_event_formatter::begin(
    std::ostream& os, 
    events::exec_event const& source_event, 
    root_path_info const& root_info,
    std::optional<parent_path_info> const& parent_info
) const {
    begin_html(os);
    format_page_header(os, source_event, root_info, parent_info);
    begin_event_table(os);
    os.flush();
}

static void end_html(std::ostream& os) { os << "</tbody></table></body></html>"; }

void html_event_formatter::end(std::ostream& os) const {
    end_html(os);
    os.flush();
}

void html_event_formatter::format(std::ostream& os, fork_event const& e) const {
    // We ignore forks
}

void html_event_formatter::format(std::ostream& os, exit_event const& e) const {
    os << "<tr class='event'>"
        << "<td class='timestamp'>" << round_to_millis(e.timestamp) << "</td>"
        << "<td>" << "EXIT " << e.exit_code << "</td>"
        << "</tr>";

    os << "<script>"
        << "document.getElementById('exit_code').textContent = " << "'exit " << e.exit_code << "'"
        << "</script>";
    os.flush();
}

void html_event_formatter::child_exit(std::ostream& os, exit_event const& e) const {
    os << "<script>"
        << "if (document.getElementById('child_exit_code_" << e.source_pid << "'))"
        << "document.getElementById('child_exit_code_" << e.source_pid << "').innerHTML = " << "'exit&nbsp;" << e.exit_code << "&nbsp;'"
        << "</script>";
}

void html_event_formatter::format(std::ostream& os, exec_event const& e, std::filesystem::path child_link) const {
    os << "<tr class='event'>"
        << "<td class='timestamp'>" << round_to_millis(e.timestamp) << "</td>"
        << "<td>"
        << "<span style='display: flex;gap: 5px;'>"
        << "<span id='child_exit_code_" << e.source_pid << "'>exit&nbsp;?&nbsp;</span>"
        << "<a href='./" << child_link.string() << "'>"
        << e.command
        << "</a>"
        << "</span></td>"
        << "</tr>";
    os.flush();
}

static std::string convert_spaces(std::string const& str) {
    // Consecutive spaces in HTML are collapsed into a single character
    // Normally, we could use <pre> which preserves them, however this tag does not work well in Lynx
    // Instead, we convert all spaces to &nbsp which is not collapsed
    return std::regex_replace(str, std::regex{" "}, "&nbsp");
}

static std::string remove_ansi_encoding(std::string const& str) {
    // This regex comes from the following site: https://stackoverflow.com/questions/14693701/how-can-i-remove-the-ansi-escape-sequences-from-a-string-in-python
    // Published by Martijn Pieters on CC-BY-SA license.
    std::regex regex("\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])");
    return std::regex_replace(str, regex, "");
}

void html_event_formatter::format(std::ostream& os, write_event const& e) const {
    std::string style = e.file_descriptor == write_event::descriptor::STDERR ? "style='color: #f5a142;'" : "";

    os << "<tr class='event'>"
        << "<td class='timestamp'>" << round_to_millis(e.timestamp) << "</td>"
        << "<td><span " << style << ">" << remove_ansi_encoding(e.data) << "</td></span>"
        << "</tr>";
    os.flush();
}
