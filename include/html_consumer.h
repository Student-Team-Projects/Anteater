#pragma once

#include <chrono>

#include "consumer.h"
#include "provider.h"

#include "html/HtmlViewFactory.h"
#include "presentation/MainPresenter.h"
#include "presentation/PresenterFactory.h"

class HtmlConsumer : public Consumer {
    pid_t root_pid;
    volatile bool exiting = false;
    bool hex_input;
    bool capture_added = false;

    /* Objects from frontend module */
    std::unique_ptr<HtmlViewFactory> view_factory;
    std::unique_ptr<PresenterFactory> presenter_factory;
    std::unique_ptr<MainPresenter> main_presenter;

public:
    HtmlConsumer() = default;
    int start(Provider &provider, pid_t root_pid, bool hex_input) override;
    void stop() override;

private:
    /* Converts timestamp from epoch */
    std::chrono::system_clock::time_point toSystemClockTimePoint(uint64_t event_timestamp);
    int consume(Provider &provider);
};
