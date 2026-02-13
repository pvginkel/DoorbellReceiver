#pragma once

#include "ApplicationBase.h"
#include "Device.h"
#include "LDR.h"
#include "SEN0626.h"
#include "WS2812StatusLed.h"

class Application : public ApplicationBase {
    struct DeviceState {
        bool enabled;
    };

    DeviceState _state{};
    WS2812StatusLed _status_led;
    Device _device;

protected:
    void do_begin() override;
    void do_ready() override;
    void do_configuration_loaded(cJSON* data) override;
    void do_process() override;

private:
    void state_changed();
    void publish_mqtt_discovery();
    void load_state();
    void save_state();
};
