#include <csignal>
#include <cstdio>

#include <future>
#include <functional>
#include <iostream>
#include <pwd.h>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "tclap/CmdLine.h"

#include "bpf_provider.h"
#include "consumer.h"
#include "sysdig_provider.h"

std::function<void(int)> exit_handler;

void sig_handler(int signal) {
    exit_handler(signal);
}

int main(int argc, const char **argv) {
    try {
        // Setting up command line parsing using TCLAP library from https://github.com/mirror/tclap
        TCLAP::CmdLine cmd("Program for debugging and splitting logs.", ' ', "1.0");
        TCLAP::ValueArg<std::string> logDirArg(
            "l", "logp", 
            "Path to logs.", 
            false, 
            std::string(LOGSDIR) + "/logs_" + std::to_string(time(nullptr)) + ".txt",
            "PATH", 
            cmd
        );
        TCLAP::ValueArg<std::string> userArg(
            "u", "user", 
            "User to run the debugged program as.", 
            false, 
            "", 
            "USER", 
            cmd
        );
        TCLAP::SwitchArg sysdigArg(
            "d", "sysdig",
            "Use sysdig-based tracer that doesn't require eBPF.", 
            cmd
        );
        TCLAP::UnlabeledMultiArg<std::string> progExecArg(
            "args", // name required in constructor for some reason?
            "Program execution to debug", 
            true,
            "PROG ARGS",
            cmd
        );
        cmd.parse(argc, argv);

        // Update argv to have just the program execution args
        for (int i = 0; i < progExecArg.getValue().size(); ++i)
            argv[i] = progExecArg.getValue()[i].c_str();
        argv[progExecArg.getValue().size()] = nullptr;


        // Setting up logs using spdlog library from https://github.com/gabime/spdlog    
        auto logger = spdlog::basic_logger_mt("file_logger", logDirArg.getValue());
        spdlog::set_default_logger(logger);
        spdlog::set_pattern("[%d/%m/%Y %T%z][%-20!s:%-4!# %-10!!][%-5!l] %v");

        SPDLOG_INFO("Starting debugger execution");
    
        sigset_t sig_usr, default_set;
        sigemptyset(&sig_usr);
        sigaddset(&sig_usr, SIGUSR1);
        sigprocmask(SIG_BLOCK, &sig_usr, &default_set);

        uid_t prog_uid = 0;

        if (userArg.isSet()) {
            passwd *pwd = getpwnam(userArg.getValue().c_str());
            if (pwd == nullptr) {
                SPDLOG_ERROR("User " + userArg.getValue() + " does not exist.");
                return 1;    
            }
            prog_uid = pwd->pw_uid;
        }

        // Fork to run child and tracer
        pid_t pid = fork();
        if (!pid) {
            struct sigaction resume;
            resume.sa_handler = [](int signal){};
            resume.sa_flags = 0;
            sigaction(SIGUSR1, &resume, nullptr);

            if (prog_uid) {
                setuid(prog_uid);
                SPDLOG_INFO("Setting UID for program to " + std::to_string(prog_uid) + " (" + userArg.getValue() + ")");
            }

            SPDLOG_INFO("Program process going to sleep");
            
            sigsuspend(&default_set);
            sigprocmask(SIG_SETMASK, &default_set, nullptr);

            int err = execvp(*argv, const_cast<char **>(argv));

            SPDLOG_ERROR("Failed to exec into program to debug with error " 
                + std::to_string(err));
            return 1;
        }

        sigprocmask(SIG_SETMASK, &default_set, nullptr);

        Consumer consumer(pid);

        Provider* provider_ptr = nullptr;
        if (sysdigArg.getValue())
            provider_ptr = new SysdigProvider(pid);
        else
            provider_ptr = new BPFProvider(pid);

        exit_handler = [&](int signal) { 
            consumer.stop();
            provider_ptr->stop();
        };

        // Cleaner handling of Ctrl-C
        std::signal(SIGTERM, sig_handler);
        std::signal(SIGINT, sig_handler);

        std::thread consumer_thread = std::thread([&]() {
            consumer.start(
                *provider_ptr,
                (sysdigArg.getValue()) ? true : false // BPF doesn't convert buffers to hex yet
            );
        });

        int ret = provider_ptr->start();
        provider_ptr->stop();
        consumer_thread.join();

        delete provider_ptr;

        return ret;

    } catch (TCLAP::ArgException &e) {
        SPDLOG_ERROR(e.error() + " for arg " + e.argId());
    }
    return 1;
}
