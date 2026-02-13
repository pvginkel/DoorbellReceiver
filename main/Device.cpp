#include "support.h"

#include "Device.h"

LOG_TAG(Device);

esp_err_t Device::begin(int pin) {
    gpio_config_t btn_config = {
        .pin_bit_mask = (1ull << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_RETURN(gpio_config(&btn_config));

    _bounce.setInverted(true);
    _bounce.interval(50);
    _bounce.attach(pin);

    return ESP_OK;
}

void Device::process() {
    _bounce.update();

    if (_bounce.read()) {
        if (!_ringing && _enabled) {
            _ringing = true;

            _pressed.call();
        }
    } else {
        _ringing = false;
    }
}
