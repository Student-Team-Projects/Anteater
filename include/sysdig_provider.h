#pragma once

#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <string>
#include <queue>

#include "constants.h"
#include "event.h"
#include "provider.h"
#include "synchronized_queue.h"

class SysdigProvider : public Provider {
    synchronized_queue<event_ref> refs;

    int pipe_fds[2];
    pid_t root_pid;
    volatile bool exiting = false;

  public:
    SysdigProvider(pid_t root_pid, size_t capacity = DEFAULT_QUEUE_CAPACITY) 
        : refs(capacity), root_pid(root_pid) {}
    
    inline pid_t get_root() const { return root_pid; };
    event_ref provide() override;
    int start() override;
    void stop() override { 
      std::cerr << "[Provider] Stopping provider\n";
      exiting = true; 
    }

  private:
    int parse_and_push(std::string line, ssize_t len);
    std::map<std::string, std::string> map_line_to_params(std::string &line, ssize_t len);
    int cleanup(int err, char *buffer);
    int init();
    int listen();
};