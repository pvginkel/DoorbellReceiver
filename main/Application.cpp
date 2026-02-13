#include "support.h"

#include "Application.h"

#include "MQTTSupport.h"
#include "NVSProperty.h"
#include "driver/i2c.h"
#include "nvs_flash.h"

LOG_TAG(Application);

static NVSPropertyI1 nvs_enabled("enabled");

void Application::do_begin() {
    load_state();

    _status_led.begin();

    _status_led.set_color(Colors::Blue);
    _status_led.set_mode(StatusLedMode::Blinking, 400);

    get_mqtt_connection().on_publish_discovery([this]() { publish_mqtt_discovery(); });

    get_mqtt_connection().on_connected_changed([this](auto state) {
        if (state.connected) {
            state_changed();
        }
    });
}

void Application::do_configuration_loaded(cJSON* data) { _status_led.set_color(Colors::Green); }

void Application::do_ready() {
    _status_led.set_color(Colors::Green);
    _status_led.set_mode(StatusLedMode::Continuous, 3000);

    ESP_LOGI(TAG, "Startup complete");

    _device.on_pressed([this]() {
        ESP_LOGI(TAG, "Doorbell pressed");

        get_mqtt_connection().send_trigger("action", "pressed");
    });

    _device.set_enabled(_state.enabled);

    ESP_ERROR_CHECK(_device.begin(CONFIG_DEVICE_DOORBELL_PIN));
}

void Application::do_process() {
    _status_led.process();
    _device.process();
}

void Application::state_changed() {
    save_state();

    if (!get_mqtt_connection().is_connected()) {
        return;
    }

    // Signal activity.
    if (!_status_led.is_active()) {
        _status_led.set_mode(StatusLedMode::Continuous, 1);
    }

    const auto json = cJSON_CreateObject();
    ESP_ASSERT_CHECK(json);
    DEFER(cJSON_Delete(json));

    cJSON_AddStringToObject(json, "enabled", print_switch_state(_state.enabled ? SwitchState::ON : SwitchState::OFF));

    get_mqtt_connection().send_state(json);
}

void Application::publish_mqtt_discovery() {
    get_mqtt_connection().publish_button_discovery(
        {
            .name = "Identify",
            .object_id = "identify",
            .entity_category = "config",
            .device_class = "identify",
        },
        []() { ESP_LOGI(TAG, "Requested identification"); });

    get_mqtt_connection().publish_button_discovery(
        {
            .name = "Restart",
            .object_id = "restart",
            .entity_category = "config",
            .device_class = "restart",
        },
        []() {
            ESP_LOGI(TAG, "Requested restart");

            esp_restart();
        });

    get_mqtt_connection().publish_switch_discovery(
        {
            .name = "Enable device",
            .object_id = "enable_device",
        },
        {
            .value_template = "{{ value_json.enabled }}",
        },
        [this](bool value) {
            ESP_LOGI(TAG, "Set device enabled: %d", value);

            _state.enabled = value;
            _device.set_enabled(value);

            state_changed();
        });

    get_mqtt_connection().publish_device_automation({
        .trigger_name = "action",
        .trigger_value = "pressed",
    });
}

void Application::load_state() {
    ESP_LOGD(TAG, "Loading state");

    _state.enabled = true;

    nvs_handle_t handle;
    auto err = nvs_open("storage", NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return;
    }
    ESP_ERROR_CHECK(err);

    DEFER(nvs_close(handle));

    _state.enabled = nvs_enabled.get(handle, false);
}

void Application::save_state() {
    ESP_LOGD(TAG, "Saving state");

    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &handle));
    DEFER(nvs_close(handle));

    nvs_enabled.set(handle, _state.enabled);
}
