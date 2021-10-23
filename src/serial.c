#include "serial.h"

#include <stddef.h>
#include <stdio.h>
#include <debug.h>

#include <usbdrvce.h>
#include <srldrvce.h>

#define SERIAL_BUF_SIZE 4096

struct event_callback_data {
    srl_device_t *srl;
    bool *has_device;
    char *buf;
    struct terminal_state *term;
};

struct srl_callback_data {
    srl_device_t *srl;
    bool *has_device;
};

static usb_error_t handle_usb_event(usb_event_t event, void *event_data,
                                    usb_callback_data_t *callback_data) {
    const struct event_callback_data *cb_data = callback_data;
    if(event == USB_DEVICE_CONNECTED_EVENT && !(usb_GetRole() & USB_ROLE_DEVICE)) {
        usb_device_t device = event_data;
        write_string(cb_data->term, "connected\r\n");
        usb_ResetDevice(device);
    }
    if((event == USB_DEVICE_ENABLED_EVENT && !(usb_GetRole() & USB_ROLE_DEVICE)) || event == USB_HOST_CONFIGURE_EVENT) {
        if(!*cb_data->has_device) {
            usb_device_t device = event_data;
            /* Initialize the serial library with the newly attached device */
            srl_error_t error = srl_Open(cb_data->srl, device, cb_data->buf, SERIAL_BUF_SIZE, SRL_INTERFACE_ANY, 9600);

            if(error) {
#ifdef DEBUG
                char buf[50];
                sprintf(buf, "Error %u initting serial\r\n", error);
                write_string(cb_data->term, buf);
#else
                write_string(cb_data->term, "Error initting serial\r\n");
#endif
            } else {
                *cb_data->has_device = true;
            }
        }
    }
    if(event == USB_DEVICE_DISCONNECTED_EVENT) {
        dbg_printf("Device disconnected.\n");
        *cb_data->has_device = false;
    }
    return USB_SUCCESS;
}

static void serial_out(const char *str, size_t len, void *data) {
    const struct srl_callback_data *srl_out = data;
    if(*srl_out->has_device) {
        srl_Write(srl_out->srl, str, len);
    }
}

bool init_serial(struct terminal_state *term) {
    static srl_device_t srl;
    static bool has_srl_device = false;
    static char srlbuf[SERIAL_BUF_SIZE];
    static struct srl_callback_data term_cb_data = {&srl, &has_srl_device};
    static struct event_callback_data usb_cb_data = {
            &srl,
            &has_srl_device,
            srlbuf,
            NULL
    };

    usb_cb_data.term = term;

    usb_error_t error = usb_Init(handle_usb_event, &usb_cb_data, srl_GetCDCStandardDescriptors(), USB_DEFAULT_INIT_FLAGS);
    if(error) return false;

    term->input_callback = serial_out;
    term->callback_data = &term_cb_data;

    usb_HandleEvents();
    return true;
}

void process_serial(struct terminal_state *term) {
    const struct srl_callback_data *cb_data = term->callback_data;

    usb_HandleEvents();

    if(*cb_data->has_device) {
        char buf[64];
        size_t len = srl_Read(cb_data->srl, buf, sizeof buf);
        write_data(term, buf, len);
    }
}
