#include "sysdig_provider.h"

#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <csignal>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "constants.h"
#include "event.h"
#include "utils.h"

int SysdigProvider::start() {
    int ret = init();
    if (!ret) ret = listen();
    return ret;
}

int SysdigProvider::init() {
    pipe(pipe_fds);

    if (fork() == 0) {
        close(pipe_fds[0]);
        dup2(pipe_fds[1], STDOUT_FILENO);
        close(pipe_fds[1]);

        std::cerr << "[Provider] Starting sysdig\n";

        execlp(
            SUDO, 
            SUDO, 
            SYSDIG, 
            "-c", 
            CHISEL, 
            std::to_string(root_pid).c_str(),
            nullptr
        );
        
        return 1;
    }

    return 0;
}

int SysdigProvider::listen() {
    // Process events
    close(pipe_fds[1]);
    FILE *sysdig_info = fdopen(pipe_fds[0], "r");

    char *buffer = nullptr;
    size_t buffer_size = 0;
    ssize_t len;

    std::cerr << "[Provider] Starting processing events\n";
    
    while (!exiting && (len = getline(&buffer, &buffer_size, sysdig_info)) != -1) 
        parse_and_push(buffer, len);

    std::cerr << "[Provider] Events processing finished\n";

    // Cleanup after all events have been processed
    cleanup(0, buffer);

    return 0;
}

int SysdigProvider::parse_and_push(std::string line, ssize_t len) {
    std::map<std::string, std::string> params = map_line_to_params(line, len);

    size_t buf_len = 0;
    if (params["type"] == "EXECVE")
        buf_len = params["name"].length();
    else if (params["type"] == "WRITE")
        buf_len = params["data"].length();

    size_t sz = sizeof(Event) + buf_len;
    Event *pt = static_cast<Event *>(malloc(sz));

    if (params["pid"] == "nil") {
        std::cerr << "[Provider] Encountered event with nil pid!\n";
        return 1;
    }

    pt->pid = stoi(params["pid"]);
    // params["time"] is in exponential notation
    pt->timestamp = (uint64_t)stod(params["time"]);
	
    if (params["type"] == "FORK") {
        pt->event_type = FORK;
        if (params["ppid"] == "nil") {
            std::cerr << "[Provider] Encountered fork event with nil ppid!\n";
            return 1;
        }
        // TODO: hacky, would be better if pid and ppid where swapped in chisel
        pt->pid = stoi(params["ppid"]);
        pt->fork.child_pid = stoi(params["pid"]);
    } else if (params["type"] == "PROCEXIT") {
        // TODO: This doesn't work, incorrect exit code
        pt->event_type = EXIT;
        pt->exit.code = (unsigned)(123);
    } else if (params["type"] == "WRITE") {
        pt->event_type = WRITE;
        pt->write.length = buf_len;
        memcpy(&pt->write.data, params["data"].data(), buf_len);
    } else if (params["type"] == "EXECVE") {
        pt->event_type = EXEC;
        pt->exec.length = buf_len;
        memcpy(&pt->exec.data, params["name"].data(), buf_len);
    } else {
        std::cerr << "[Provider] Unknown event type " << params["type"] << "\n";
    }

    refs.push(event_ref(pt));

    return 0;
}

std::map<std::string, std::string> SysdigProvider::map_line_to_params(std::string &line, ssize_t len) {
    std::map<std::string, std::string> params;

    std::vector<std::string> line_split;
    
    std::stringstream ss(line);
    std::string element;

    while (getline(ss, element, ' '))
        line_split.push_back(element);

    for (auto &param : line_split) {
        size_t split_point = param.find('=', 0);
        params[param.substr(0, split_point)] = param.substr(split_point+1, param.length()-split_point);
    }

    return params;
}

int SysdigProvider::cleanup(int err, char *buffer) {
    std::cerr << "[Provider] Cleanup\n";

    refs.unblock();
    free(buffer);
    while (wait(nullptr) != -1);
    close(pipe_fds[0]);

    return 0;
}

Provider::event_ref SysdigProvider::provide() {
    return refs.pop();
}
