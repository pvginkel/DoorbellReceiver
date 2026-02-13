#pragma once

#include "Bounce2.h"
#include "Callback.h"

class Device {
    Callback<void> _pressed;
    Bounce _bounce;
    bool _enabled{};
    bool _ringing{};

public:
    esp_err_t begin(int pin);
    void process();

    void set_enabled(bool enabled) { _enabled = enabled; }

    void on_pressed(std::function<void()> func) { _pressed.add(std::move(func)); }
};
