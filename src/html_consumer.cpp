#include "html_consumer.h"
#include "utils.h"

#include "spdlog/spdlog.h"

#include <chrono>
#include <cstdio>
#include <unordered_set>

#include "html/HtmlViewFactory.h"
#include "presentation/MainPresenter.h"
#include "presentation/PresenterFactory.h"

int HtmlConsumer::start(Provider &provider, pid_t root_pid, bool hex_input) {
    this->root_pid = root_pid;
    this->hex_input = hex_input;
    running_processes.insert(root_pid);

    const std::filesystem::path directory =  "/var/lib/debugger";
    std::filesystem::create_directory(directory);
    this->view_factory = std::make_unique<HtmlViewFactory>(directory, "index", "styles", true);
    this->presenter_factory = std::make_unique<PresenterFactory>(std::move(view_factory));
    this->main_presenter = presenter_factory->createMainPresenter();

    SPDLOG_INFO("Starting html consumer");
    
    while (!exiting) consume(provider);

    return 0;
}

std::chrono::system_clock::time_point HtmlConsumer::toSystemClockTimePoint(uint64_t event_timestamp) {
    using namespace std::chrono;
    auto duration_ns = duration<uint64_t, std::nano>(event_timestamp);
    return system_clock::time_point(duration_cast<system_clock::duration>(duration_ns));
}

int HtmlConsumer::consume(Provider &provider) {
    Provider::event_ref event = provider.provide();

    if (exiting)
        return 0;

    if (!event) {
        SPDLOG_ERROR("Got null pointer from provider. Stopping consuming");
        stop();
        return 1;
    }

    auto timestamp = toSystemClockTimePoint(event->timestamp);

    if(!capture_added){
        if(event->event_type != EXEC) {
            SPDLOG_ERROR("Expected EXEC as first event. Exiting...");
            stop();
            return 1;
        }
        convert_spaces(event->exec.data, event->exec.length);
        main_presenter->addCapture(
            timestamp,
            event->pid,
            event->exec.uid,
            std::string(event->exec.data, event->exec.length));
        capture_added=true;
        return 0;
    }

    switch (event->event_type) {
        case FORK:
            running_processes.insert(event->fork.child_pid);
            main_presenter->addForkEvent(timestamp, event->pid, event->fork.child_pid);
            return 0;
        case EXIT:
            running_processes.erase(event->pid);
            main_presenter->addExitEvent(timestamp, event->pid, event->exit.code);
            if (running_processes.empty()) {
                stop();
                provider.stop();
            }
            return 0;
        case WRITE:
            main_presenter->addWriteEvent(
                timestamp,
                event->pid,
                std::string_view(event->write.data, event->write.length),
                event->write.stream == Stream::STDOUT
            );
            return 0;
        case EXEC:
            convert_spaces(event->exec.data, event->exec.length);
            main_presenter->addExecEvent(
                timestamp,
                event->pid,
                event->exec.uid,
                std::string_view(event->exec.data, event->exec.length)
            );
            return 0;
        default:
            stop();
            provider.stop();
    }

    return 0;
}

void HtmlConsumer::stop() {
    SPDLOG_INFO("Stopping html consumer");
    exiting = true;
}
