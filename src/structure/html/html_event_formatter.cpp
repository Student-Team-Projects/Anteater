#include "structure/html/html_event_formatter.hpp"

using namespace events;

const std::string CSS = R"(
    <style>
        html { font-family: monospace; }
        body {
            background: #111;
            color: #eee;
        }
        pre {
            margin: 0;
        }
        td {
            padding: 2px 10px;
        }
        .event > :first-child {
            border-right: 1px solid #eee;
        }
        a {
            color: green;
        }
    </style>
)";

void format_page_header(std::ostream& os, exec_event const& source_event) {
    os << "<table><tbody>";

    os << "<tr>" << "<td> account </td>" << "<td>" << source_event.user_name << "</td>" << "</tr>";
    os << "<tr>" << "<td> command </td>" << "<td>" << source_event.command << "</td>" << "</tr>";
    os << "<tr>" << "<td> working directory </td>" << "<td>" << source_event.working_directory << "</td>" << "</tr>";
    os << "<tr>" << "<td> timestamp </td>" << "<td>" << source_event.timestamp << "</td>" << "</tr>";
    os << "<tr>" << "<td> exit code </td>" << "<td id='exit_code'>" << "?" << "</td>" << "</tr>";

    os << "</tbody></table>";
    os << "<hr>";
}

void html_event_formatter::begin(std::ostream& os, exec_event const& source_event) {
    os << "<!DOCTYPE HTML>"
        << "<html lang='pl-PL'>"

        << "<head>"
        << "<meta charset='UTF-8'/>"
        << CSS
        << "</head>"

        << "<body>";

    format_page_header(os, source_event);

    os << "<table><tbody>";
    os.flush();
}

void html_event_formatter::end(std::ostream& os) {
    os << "</body></table></html>";
    os.flush();
}

void html_event_formatter::format(std::ostream& os, fork_event const& e) {
    // We ignore forks
}

void html_event_formatter::format(std::ostream& os, exit_event const& e) {
    os << "<tr class='event'>"
        << "<td>" << e.timestamp << "</td>"
        << "<td>" << "EXIT " << e.exit_code << "</td>"
        << "</tr>";

    os << "<script>"
        << "document.getElementById('exit_code').textContent = " << "'exit " << e.exit_code << "'"
        << "</script>";
    os.flush();
}

void html_event_formatter::child_exit(std::ostream& os, exit_event const& e) {
    os << "<script>"
        << "if (document.getElementById('child_exit_code_" << e.source_pid << "'))"
        << "document.getElementById('child_exit_code_" << e.source_pid << "').textContent = " << "'exit " << e.exit_code << "'"
        << "</script>";
}

void html_event_formatter::format(std::ostream& os, exec_event const& e, std::filesystem::path child_link) {
    os << "<tr class='event'>"
        << "<td>" << e.timestamp << "</td>"
        << "<td>"
        << "<span style='display: flex;gap: 5px;'>"
        << "<span id='child_exit_code_" << e.source_pid << "'>exit ?&nbsp</span>"
        << "<a href='./" << child_link.string() << "'>"
        << e.command
        << "</a>"
        << "</span></td>"
        << "</tr>";
    os.flush();
}

void html_event_formatter::format(std::ostream& os, write_event const& e) {
    std::string style = e.file_descriptor == write_event::descriptor::STDERR ? "style='color: #f5a142;'" : "";

    os << "<tr class='event'>"
        << "<td>" << e.timestamp << "</td>"
        << "<td><span " << style << ">" << e.data << "</td></span>"
        << "</tr>";
    os.flush();
}