// ties to the compatible = "custom,button" node in the device tree
#define DT_DRV_COMPAT custom_button // zephyr looks for this macro, we have to write exactly like this

#include <errno.h>
#include <zephyr/logging/log.h>

#include "button.h"

// enable logging
LOG_MODULE_REGISTER(button);

// -----------------------------------------------------------------------
static int button_init(const struct device *dev);
static int get_button_state(const struct device *dev, uint8_t *state);
// -----------------------------------------------------------------------
// Private Functions
static int button_init(const struct device *dev) {
    int ret;

    // cast the device config to our button_config struct, check device struct
    const struct button_config *cfg = (const struct button_config *)dev->config;
    // get the button struct from the config
    const struct gpio_dt_spec *btn = &cfg->btn;
    
    LOG_DBG("Initializing button (ID = %u)\r\n", cfg->id);
    // check if the button is ready
    if (!gpio_is_ready_dt(btn)) {
	LOG_ERR("GPIO is not ready\r\n");
	return -ENODEV;
    }

    // setup button as input
    ret = gpio_pin_configure_dt(btn, GPIO_INPUT);
    if (ret < 0) {
	LOG_ERR("Couldn't configure GPIO as input\r\n");
	return -ENODEV;
    }

    return 0;
}
// -----------------------------------------------------------------------
// Public functions (API)

static int get_button_state(const struct device *dev, uint8_t *state) {
    int ret;

    // cast the device config to our button_config struct, check device struct
    const struct button_config *cfg = (const struct button_config *)dev->config;
    // get the button struct from the config
    const struct gpio_dt_spec *btn = &cfg->btn;

    // poll the button's state
    ret = gpio_pin_get_dt(btn);
    if (ret < 0) {
	LOG_ERR("Error (%d): failed to read the button pin\r\n", ret);
	return ret;
    } else {
	*state = ret;
    }

    return 0;
}

// -----------------------------------------------------------------------
// Device Tree Handling

// define public API fuctions for the driver
static const struct button_api button_api_funcs = {
    .get = get_button_state,
};


#define BUTTON_DEFINE(inst)                                                 \
                                                                            \
    /* Create an instance of the config struct, populate with DT values */  \
    static const struct button_config button_config_##inst = {              \
        .btn = GPIO_DT_SPEC_GET(                                            \
            DT_PHANDLE(DT_INST(inst, custom_button), pin), gpios),          \
        .id = inst                                                          \
    };                                                                      \
                                                                            \
    /* Create a "device" instance from a Devicetree node identifier and */  \
    /* registers the init function to run during boot. */                   \
    DEVICE_DT_INST_DEFINE(inst,                                             \
                          button_init,                                      \
                          NULL,                                             \
                          NULL,                                             \
                          &button_config_##inst,                            \
                          POST_KERNEL,                                      \
                          CONFIG_GPIO_INIT_PRIORITY,                        \
                          &button_api_funcs);                               \

// The Devicetree build process calls this to create an instance of structs for
// each device (button) defined in the Devicetree
DT_INST_FOREACH_STATUS_OKAY(BUTTON_DEFINE)
