#include <csignal>
#include <cstdio>

#include <future>
#include <functional>
#include <iostream>
#include <pwd.h>
#include <grp.h>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "tclap/CmdLine.h"

#include "bpf_provider.h"
#include "sysdig_provider.h"

#include "html_consumer.h"
#include "plain_consumer.h"

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
        TCLAP::SwitchArg plainArg(
            "p", "plain",
            "Use to make debugger print plain text to standard output instead of creating html files.",
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
        spdlog::flush_every(std::chrono::seconds(3));
        spdlog::set_pattern("[%d/%m/%Y %T%z][%-20!s:%-4!# %-10!!][%-5!l] %v");

        SPDLOG_INFO("Starting debugger execution");
    
        sigset_t sig_usr, default_set;
        sigemptyset(&sig_usr);
        sigaddset(&sig_usr, SIGUSR1);
        sigprocmask(SIG_BLOCK, &sig_usr, &default_set);

        uid_t prog_uid = 0;
        gid_t grps[NGROUPS_MAX];
        int grp_cnt = NGROUPS_MAX;

        if (userArg.isSet()) {
            passwd *pwd = getpwnam(userArg.getValue().c_str());
            if (pwd == nullptr) {
                SPDLOG_ERROR("User " + userArg.getValue() + " does not exist.");
                return 1;    
            }
            prog_uid = pwd->pw_uid;
            getgrouplist(userArg.getValue().c_str(), pwd->pw_gid, grps, &grp_cnt);
        }

        // Fork to run child and tracer
        pid_t pid = fork();
        if (!pid) {
            struct sigaction resume;
            resume.sa_handler = [](int signal){};
            resume.sa_flags = 0;
            sigaction(SIGUSR1, &resume, nullptr);

            if (prog_uid) {
                setgroups(grp_cnt, grps);
                setuid(prog_uid);
            }

            sigsuspend(&default_set);
            sigprocmask(SIG_SETMASK, &default_set, nullptr);

            return -execvp(*argv, const_cast<char **>(argv));
        }

        sigprocmask(SIG_SETMASK, &default_set, nullptr);

        // Creating events provider
        Provider* provider_ptr = nullptr;
        if (sysdigArg.getValue())
            provider_ptr = new SysdigProvider(pid);
        else
            provider_ptr = new BPFProvider(pid);

        // Creating events consumer
        Consumer* consumer_ptr = nullptr;
        if (plainArg.getValue())
            consumer_ptr = new PlainConsumer();
        else
            consumer_ptr = new HtmlConsumer();

        // Cleaner handling of Ctrl-C
        exit_handler = [&](int signal) { 
            consumer_ptr->stop();
            provider_ptr->stop();
        };

        std::signal(SIGTERM, sig_handler);
        std::signal(SIGINT, sig_handler);

        SPDLOG_INFO("Starting consumer...");
        std::thread consumer_thread = std::thread([&]() {
            consumer_ptr->start(
                *provider_ptr,
                pid,
                (sysdigArg.getValue()) ? true : false // BPF doesn't convert buffers to hex yet
            );
        });
        SPDLOG_INFO("Starting provider...");
        int ret = provider_ptr->start();
        provider_ptr->stop();
        consumer_thread.join();

        delete provider_ptr;
        delete consumer_ptr;

        return ret;

    } catch (TCLAP::ArgException &e) {
        SPDLOG_ERROR(e.error() + " for arg " + e.argId());
    }
    return 1;
}
