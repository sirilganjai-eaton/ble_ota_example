#include "esp_log.h"
#include "esp_ota_ops.h"
#include "gap.h"
#include "gatt_svr.h"
#include "nvs_flash.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_store.h"


#define LOG_TAG_MAIN "main"
static uint16_t ble_conn_handle;
bool run_diagnostics() {
  // do some diagnostics
  return true;
}

// Security parameters
static void bleprph_set_security_params(void) {
    struct ble_hs_cfg sec_cfg = {
        .sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT, // Updated IO capabilities
        .sm_bonding = 1,
        .sm_mitm = 1,
        .sm_sc = 1,
        .sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID,
        .sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID,
    };

    ble_hs_cfg.sm_io_cap = sec_cfg.sm_io_cap;
    ble_hs_cfg.sm_bonding = sec_cfg.sm_bonding;
    ble_hs_cfg.sm_mitm = sec_cfg.sm_mitm;
    ble_hs_cfg.sm_sc = sec_cfg.sm_sc;
    ble_hs_cfg.sm_our_key_dist = sec_cfg.sm_our_key_dist;
    ble_hs_cfg.sm_their_key_dist = sec_cfg.sm_their_key_dist;
}

// Security callback
static int bleprph_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(LOG_TAG_MAIN, "/*----------Connection established*-----------/");
              ble_conn_handle = event->connect.conn_handle;
              ESP_LOGI(LOG_TAG_MAIN, "Connection handle: %d", ble_conn_handle);
              // Initiate MTU exchange
              ble_att_set_preferred_mtu(BLE_ATT_MTU_MAX);
              // my temp call
              
              int rc = ble_gattc_exchange_mtu(ble_conn_handle, NULL, NULL);
                if (rc != 0) {
                    ESP_LOGE(LOG_TAG_MAIN, "Error initiating MTU exchange; rc=%d", rc);
                }
                rc = ble_hs_hci_util_set_data_len(ble_conn_handle, 251, 0x4290);
                if (rc != 0) {
                    ESP_LOGE(LOG_TAG_MAIN, "Error setting suggested default ble_hs_hci_util_set_data_len; rc=%d", rc);
                }
                // Set preferred data length
                rc = ble_hs_hci_util_write_sugg_def_data_len(251, 0x4290);
                if (rc != 0) {
                    ESP_LOGE(LOG_TAG_MAIN, "Error setting suggested default data length; rc=%d", rc);
                }
                rc = ble_gap_set_data_len(ble_conn_handle, 251, 0x4290);
                if (rc != 0) {
                    ESP_LOGE(LOG_TAG_MAIN, "Error setting data length; rc=%d", rc);
                }
            ESP_LOGI("BLE", "Connection established");
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI("BLE", "Connection terminated");
            break;
        case BLE_GAP_EVENT_PASSKEY_ACTION:
            ESP_LOGI("BLE", "Passkey action event");
            break;
        case BLE_GAP_EVENT_ENC_CHANGE:
            ESP_LOGI("BLE", "Encryption change event");
            break;
        default:
            break;
    }
    return 0;
}

void app_main(void) {
  // check which partition is running
  const esp_partition_t *partition = esp_ota_get_running_partition();
  ESP_LOGI(LOG_TAG_MAIN, "-------------------------------------------");
  ESP_LOGI(LOG_TAG_MAIN, "This is the MAIN firmware, AKA Factory APP");
  ESP_LOGI(LOG_TAG_MAIN, "-------------------------------------------");
  switch (partition->address) {
    case 0x00010000:
      ESP_LOGI(LOG_TAG_MAIN, "Running partition: factory");
      break;
    case 0x00110000:
      ESP_LOGI(LOG_TAG_MAIN, "Running partition: ota_0");
      break;
    case 0x00210000:
      ESP_LOGI(LOG_TAG_MAIN, "Running partition: ota_1");
      break;

    default:
      ESP_LOGE(LOG_TAG_MAIN, "Running partition: unknown");
      break;
  }

  // check if an OTA has been done, if so run diagnostics
  esp_ota_img_states_t ota_state;
  if (esp_ota_get_state_partition(partition, &ota_state) == ESP_OK) {
    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
      ESP_LOGI(LOG_TAG_MAIN, "An OTA update has been detected.");
      if (run_diagnostics()) {
        ESP_LOGI(LOG_TAG_MAIN,
                 "Diagnostics completed successfully! Continuing execution.");
        esp_ota_mark_app_valid_cancel_rollback();
      } else {
        ESP_LOGE(LOG_TAG_MAIN,
                 "Diagnostics failed! Start rollback to the previous version.");
        esp_ota_mark_app_invalid_rollback_and_reboot();
      }
    }
  }

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // BLE Setup
  
  // initialize BLE controller and nimble stack
  //esp_nimble_hci_and_controller_init();



  nimble_port_init();
  
  
  //ble_gap_set_data_len(ble_hs_cfg, 0x00fb, 0x4290);
  //pritnf("Device operating in Bluetooth Mode: %d", CONFIG_BT_DEVICE_NAME);
  // register sync and reset callbacks
  
  ble_hs_cfg.sync_cb = sync_cb;
  ble_hs_cfg.reset_cb = reset_cb;
  
  // initialize service table
  gatt_svr_init();

  // set device name and start host task
  ble_svc_gap_device_name_set(device_name);

  // Set security parameters
  bleprph_set_security_params();


  // Register GAP event handler
  //ble_gap_set_event_cb(bleprph_gap_event, NULL,NULL);
  /*
  int rc = ble_hs_hci_util_write_sugg_def_data_len((uint16_t)0x00fb, (uint16_t)0x4290);
  if (rc != 0) {
    ESP_LOGE(LOG_TAG_MAIN, "Error setting suggested default data length; rc=%d", rc);
  }
  uint16_t out_sugg_max_tx_octets = 0;
  uint16_t out_sugg_max_tx_time = 0;
 ble_gap_read_sugg_def_data_len(&out_sugg_max_tx_octets, &out_sugg_max_tx_time);
  ESP_LOGI(LOG_TAG_MAIN, "Suggested Max Tx Octets: %d", out_sugg_max_tx_octets);
  ESP_LOGI(LOG_TAG_MAIN, "Suggested Max Tx Time: %d", out_sugg_max_tx_time);
 */
  //ble_gap_event_listener_register(bleprph_gap_event, NULL, NULL);
  //le_gap_event_listener_register(NULL, bleprph_gap_event, NULL);

  nimble_port_freertos_init(host_task);
  
}