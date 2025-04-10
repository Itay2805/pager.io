#include "ble.h"

#include <esp_nimble_hci.h>
#include <host/ble_hs.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

#define TAG "ble"

/**
 * Just runs the nimble stack and nothing else
 */
static void ble_host_task(void* param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void ble_advertise(void);

/**
 * Handle am event related to the connection
 */
static int ble_gap_event(struct ble_gap_event* event, void *arg) {
    // TODO: when a disconnect happens start advertizing again

    switch (event->type) {
        default: {
            ESP_LOGW(TAG, "Unknown GAP event %d", event->type);
        } break;
    }
    return 0;
}

/**
 * Advertise that we exist
 */
static void ble_advertise(void) {
    // TODO: once paired maybe do a direct advertising

    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
    };
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

/**
 * Called once the stack is ready, just start the advertizing
 */
static void ble_on_sync(void) {
    ble_advertise();
}

// Characteristic callback
static int gatt_svr_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg) {
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_WRITE_CHR: {
            // TODO: handle a new message
        } return 0;

        case BLE_GATT_ACCESS_OP_READ_CHR: {
            // TODO: return the status
        } return 0;

        default:
            return BLE_ATT_ERR_UNLIKELY;
    }
}

/**
 * We are going to setup a single characteristic that we are going
 * to pass encrypted messages on top of.
 *
 * Because the device doesn't need to tell anything special to the phone
 * other than the occasional status on connect we are going to have it
 * also be a read endpoint that would return the status of the device (also
 * encrypted)
 */
static const struct ble_gatt_svc_def m_ble_gatt_svr[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID128_DECLARE(0x8d, 0x15, 0xd4, 0x4a, 0xb, 0x4d, 0x4d, 0xf9, 0x9f, 0xf8, 0xa1, 0x50, 0x47, 0x17, 0x18, 0x92),
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = BLE_UUID128_DECLARE(0x98, 0xdc, 0xd7, 0x96, 0xe0, 0x6b, 0x44, 0xb7, 0x98, 0xb6, 0x39, 0xfe, 0x75, 0x3c, 0xc, 0x17),
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ,
                .access_cb = gatt_svr_chr_access_cb
            },
            { 0 }
        }
    },
    { 0 }
};

void init_ble(void) {
    // setup the BLE stack
    ESP_ERROR_CHECK(nimble_port_init());
    ESP_ERROR_CHECK(esp_nimble_hci_init());

    // setup the GAP and GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_gatts_count_cfg(m_ble_gatt_svr);
    ble_gatts_add_svcs(m_ble_gatt_svr);

    // get a callback when sync is done so we can advertise the GAP properly
    ble_hs_cfg.sync_cb = ble_on_sync;

    // And run the freertos thread properly
    nimble_port_freertos_init(ble_host_task);

}
