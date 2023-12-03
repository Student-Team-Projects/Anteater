#include "structure/html/html_event_formatter.hpp"

using namespace events;

const std::string CSS = R"(
    <style>
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
        tr > :first-child {
            border-right: 1px solid #eee;
        }
        a {
            color: green;
        }
    </style>
)";

void html_event_formatter::begin(std::ostream& os) {
    os << "<!DOCTYPE HTML>"
        << "<html lang='pl-PL'>"

        << "<head>"
        << "<meta charset='UTF-8'/>"
        << CSS
        << "</head>"

        << "<body>";

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
    os << "<tr>"
        << "<td><pre>" << e.timestamp << "</pre></td>"
        << "<td><pre>" << "EXIT " << e.exit_code << "</td></pre>"
        << "</tr>";
    os.flush();
}

void html_event_formatter::child_exit(std::ostream& os, exit_event const& e) {
    os << "<script>"
        << "if (document.getElementById('child_exit_code_" << e.source_pid << "'))"
        << "document.getElementById('child_exit_code_" << e.source_pid << "').textContent = " << "'exit " << e.exit_code << "'"
        << "</script>";
}

void html_event_formatter::format(std::ostream& os, exec_event const& e, std::filesystem::path child_link) {
    os << "<tr>"
        << "<td><pre>" << e.timestamp << "</pre></td>"
        << "<td>"
        << "<div style='display: flex;gap: 5px;'>"
        << "<pre id='child_exit_code_" << e.source_pid << "'>exit ?</pre>"
        << "<a href='/" << child_link.string() << "'>"
        << "<pre>" << e.command  << "</pre>"
        << "</a>"
        << "</pre></div></td>"
        << "</tr>";
    os.flush();
}

void html_event_formatter::format(std::ostream& os, write_event const& e) {
    std::string style = e.file_descriptor == write_event::descriptor::STDERR ? "style='color: #f5a142;'" : "";

    os << "<tr>"
        << "<td><pre>" << e.timestamp << "</pre></td>"
        << "<td><pre " << style << ">" << e.data << "</td></pre>"
        << "</tr>";
    os.flush();
}