#include "gatt_svr.h"

// include the header from proto dir
#include "proto/device_feature.pb-c.h"


#define DATA_SIZE 2500
#define FILL_ARRAY(arr, size) \
    for (int i = 0; i < size; i++) { \
        arr[i] = i; \
    }

uint8_t gatt_svr_chr_ota_control_val;
uint8_t gatt_svr_chr_ota_data_val[512];

uint16_t ota_control_val_handle;
uint16_t ota_data_val_handle;

const esp_partition_t *update_partition;
esp_ota_handle_t update_handle;
bool updating = false;
uint16_t num_pkgs_received = 0;
uint16_t packet_size = 0;

static const char *manuf_name = "EATON";
static const char *model_num = "ESP32_Test_Device";




// fill this data with the index
uint8_t data_chunk[DATA_SIZE]= {0};

static int gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len,
                              uint16_t max_len, void *dst, uint16_t *len);

static int gatt_svr_chr_ota_control_cb(uint16_t conn_handle,
                                       uint16_t attr_handle,
                                       struct ble_gatt_access_ctxt *ctxt,
                                       void *arg);

static int gatt_svr_chr_ota_data_cb(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt,
                                    void *arg);

static int gatt_svr_chr_access_device_info(uint16_t conn_handle,
                                           uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt *ctxt,
                                           void *arg);

static int gatt_svr_chr_access_512_bytes(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg);


//device features
static int gatt_svr_chr_access_device_features(uint16_t conn_handle,
                                           uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt *ctxt,
                                           void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
  /*
  
  static const ble_uuid16_t gatt_svr_svc_get_device_info_uuid = BLE_UUID16_INIT(GATT_DEVICE_INFO_UUID);
static const ble_uuid16_t gatt_svr_chr_get_manufacturer_name_uuid = BLE_UUID16_INIT(GATT_MANUFACTURER_NAME_UUID);
static const ble_uuid16_t gatt_svr_chr_get_model_number_uuid = BLE_UUID16_INIT(GATT_MODEL_NUMBER_UUID);
  
  * */
    {// Service: Device Information
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_get_device_info_uuid.u,
        .characteristics =
         (struct ble_gatt_chr_def[]){
             {
                 // Characteristic: Manufacturer Name
                 .uuid = &gatt_svr_chr_get_manufacturer_name_uuid.u,//BLE_UUID16_DECLARE(GATT_MANUFACTURER_NAME_UUID),
                 .access_cb = gatt_svr_chr_access_device_info,
                 .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
             },
             {
                 // Characteristic: Model Number
                 .uuid = &gatt_svr_chr_get_model_number_uuid.u,//BLE_UUID16_DECLARE(GATT_MODEL_NUMBER_UUID),
                 .access_cb = gatt_svr_chr_access_device_info,
                 .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
             },
             {
                 0,
             },
         }},

    {
        // service: OTA Service
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_ota_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){
                {
                    // characteristic: OTA control
                    .uuid = &gatt_svr_chr_ota_control_uuid.u,
                    .access_cb = gatt_svr_chr_ota_control_cb,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE |
                             BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &ota_control_val_handle,
                },
                {
                    // characteristic: OTA data
                    .uuid = &gatt_svr_chr_ota_data_uuid.u,
                    .access_cb = gatt_svr_chr_ota_data_cb,
                    .flags = BLE_GATT_CHR_F_WRITE,
                    .val_handle = &ota_data_val_handle,
                },
                {
                    0,
                }},
    },

    {    // telemetry
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x180D), // Example UUID for Heart Rate Service
        .characteristics = (struct ble_gatt_chr_def[]) { {
            .uuid = BLE_UUID16_DECLARE(0x2A37), // Example UUID for Heart Rate Measurement Characteristic
            .access_cb = gatt_svr_chr_access_512_bytes,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY, // Read and Notify flags
        }, {
            0, // No more characteristics in this service
        } },
    },
    {
        // device features
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_get_device_features_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){
                {
                    // characteristic: Device Type
                    .uuid = &gatt_svr_chr_get_device_type_uuid.u,
                    .access_cb = gatt_svr_chr_access_device_features,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                },
                {
                    // characteristic: Voltage Rating
                    .uuid = &gatt_svr_chr_get_voltage_rating_uuid.u,
                    .access_cb = gatt_svr_chr_access_device_features,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                },
                {
                    // characteristic: Current Rating
                    .uuid = &gatt_svr_chr_get_current_rating_uuid.u,
                    .access_cb = gatt_svr_chr_access_device_features,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                },
                {
                    0,
                }},
    },
    {
        0, // No more services
    },
};

static int gatt_svr_chr_access_device_info(uint16_t conn_handle,
                                           uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt *ctxt,
                                           void *arg) {
  uint16_t uuid;
  int rc;
  ESP_LOGI(LOG_TAG_GATT_SVR, "Inside %s",__func__);
  uuid = ble_uuid_u16(ctxt->chr->uuid);

  if (uuid == GATT_MODEL_NUMBER_UUID) {
    rc = os_mbuf_append(ctxt->om, model_num, strlen(model_num));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }

  if (uuid == GATT_MANUFACTURER_NAME_UUID) {
    rc = os_mbuf_append(ctxt->om, manuf_name, strlen(manuf_name));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }

  assert(0);
  return BLE_ATT_ERR_UNLIKELY;
}

static int gatt_svr_chr_access_512_bytes(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg) {
    static int data_offset = 0;
    uint16_t mtu = ble_att_mtu(conn_handle);
    uint16_t chunk_size = mtu - 3; // Subtract 3 bytes for ATT header
    uint16_t remaining = DATA_SIZE - data_offset;

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        uint16_t len = remaining < chunk_size ? remaining : chunk_size;
        int rc = os_mbuf_append(ctxt->om, data_chunk + data_offset, len);
        if (rc != 0) {
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        data_offset += len;
        if (data_offset >= DATA_SIZE) {
            data_offset = 0; // Reset offset after sending all data
        }
        return 0;
    }

    return BLE_ATT_ERR_UNLIKELY;
}


static int gatt_svr_chr_access_device_features(uint16_t conn_handle,
                                           uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt *ctxt,
                                           void *arg) 
{
  uint16_t uuid;
  int rc;
  ESP_LOGI(LOG_TAG_GATT_SVR, "Inside %s",__func__);
  uuid = ble_uuid_u16(ctxt->chr->uuid);

  //char *device_type = "Breaker";
  const int voltage_rating = 240;
  const int current_rating = 60;

  // actual
  /*if (uuid == GATT_DEVICE_TYPE_UUID) {
    rc = os_mbuf_append(ctxt->om, device_type, strlen(device_type));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }*/
  DeviceFeature device_feature = DEVICE__FEATURE__INIT;
  device_feature.device_type = "Breaker";
  device_feature.has_voltage_rating = 1;
  device_feature.voltage_rating = 240;
  device_feature.has_current_rating = 1;
  device_feature.current_rating = 60;

  // tag 4,5 not present on the client side. yet we send them, and see what happens on the client side
  device_feature.has_wifi_support = 1;
  device_feature.wifi_support = 1;
  device_feature.has_ble_support = 1;
  device_feature.ble_support = 1;

  // serialize the structure
  size_t len = device__feature__get_packed_size(&device_feature);
  uint8_t packed[len];
  device__feature__pack(&device_feature, packed);

  // experiment
  if (uuid == GATT_DEVICE_TYPE_UUID) {
    // here, instead of device_type, it will be entire structure from protobuf
    rc = os_mbuf_append(ctxt->om, packed, len);
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }
  if (uuid == GATT_VOLTAGE_RATING_UUID) {
    rc = os_mbuf_append(ctxt->om, (const void *)&voltage_rating, sizeof(voltage_rating));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }

  if (uuid == GATT_CURRENT_RATING_UUID) {
    rc = os_mbuf_append(ctxt->om, (const void *)&current_rating, sizeof(current_rating));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }
  return 0;
}
static int gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len,
                              uint16_t max_len, void *dst, uint16_t *len) {
  uint16_t om_len;
  int rc;

  om_len = OS_MBUF_PKTLEN(om);
  if (om_len < min_len || om_len > max_len) {
    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
  }

  rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
  if (rc != 0) {
    return BLE_ATT_ERR_UNLIKELY;
  }

  return 0;
}

static void update_ota_control(uint16_t conn_handle) {
  struct os_mbuf *om;
  esp_err_t err;

  // check which value has been received
  switch (gatt_svr_chr_ota_control_val) {
    case SVR_CHR_OTA_CONTROL_REQUEST:
      // OTA request
      ESP_LOGI(LOG_TAG_GATT_SVR, "OTA has been requested via BLE.");
      // get the next free OTA partition
      update_partition = esp_ota_get_next_update_partition(NULL);
      // start the ota update
      err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES,
                          &update_handle);
      if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG_GATT_SVR, "esp_ota_begin failed (%s)",
                 esp_err_to_name(err));
        esp_ota_abort(update_handle);
        gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_REQUEST_NAK;
      } else {
        gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_REQUEST_ACK;
        updating = true;

        // retrieve the packet size from OTA data
        packet_size =
            (gatt_svr_chr_ota_data_val[1] << 8) + gatt_svr_chr_ota_data_val[0];
        ESP_LOGI(LOG_TAG_GATT_SVR, "Packet size is: %d", packet_size);

        num_pkgs_received = 0;
      }

      // notify the client via BLE that the OTA has been acknowledged (or not)
      om = ble_hs_mbuf_from_flat(&gatt_svr_chr_ota_control_val,
                                 sizeof(gatt_svr_chr_ota_control_val));
      ble_gattc_notify_custom(conn_handle, ota_control_val_handle, om);
      ESP_LOGI(LOG_TAG_GATT_SVR, "OTA request acknowledgement has been sent.");

      break;

    case SVR_CHR_OTA_CONTROL_DONE:

      updating = false;

      // end the OTA and start validation
      err = esp_ota_end(update_handle);
      if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
          ESP_LOGE(LOG_TAG_GATT_SVR,
                   "Image validation failed, image is corrupted!");
        } else {
          ESP_LOGE(LOG_TAG_GATT_SVR, "esp_ota_end failed (%s)!",
                   esp_err_to_name(err));
        }
      } else {
        // select the new partition for the next boot
        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK) {
          ESP_LOGE(LOG_TAG_GATT_SVR, "esp_ota_set_boot_partition failed (%s)!",
                   esp_err_to_name(err));
        }
      }

      // set the control value
      if (err != ESP_OK) {
        gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_DONE_NAK;
      } else {
        gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_DONE_ACK;
      }

      // notify the client via BLE that DONE has been acknowledged
      om = ble_hs_mbuf_from_flat(&gatt_svr_chr_ota_control_val,
                                 sizeof(gatt_svr_chr_ota_control_val));
      ble_gattc_notify_custom(conn_handle, ota_control_val_handle, om);
      ESP_LOGI(LOG_TAG_GATT_SVR, "OTA DONE acknowledgement has been sent.");

      // restart the ESP to finish the OTA
      if (err == ESP_OK) {
        ESP_LOGI(LOG_TAG_GATT_SVR, "Preparing to restart!");
        vTaskDelay(pdMS_TO_TICKS(REBOOT_DEEP_SLEEP_TIMEOUT));
        esp_restart();
      }

      break;

    default:
      break;
  }
}

static int gatt_svr_chr_ota_control_cb(uint16_t conn_handle,
                                       uint16_t attr_handle,
                                       struct ble_gatt_access_ctxt *ctxt,
                                       void *arg) {
  int rc;
  uint8_t length = sizeof(gatt_svr_chr_ota_control_val);

  switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR:
      // a client is reading the current value of ota control
      rc = os_mbuf_append(ctxt->om, &gatt_svr_chr_ota_control_val, length);
      return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
      break;

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
      // a client is writing a value to ota control
      rc = gatt_svr_chr_write(ctxt->om, 1, length,
                              &gatt_svr_chr_ota_control_val, NULL);
      // update the OTA state with the new value
      update_ota_control(conn_handle);
      return rc;
      break;

    default:
      break;
  }

  // this shouldn't happen
  assert(0);
  return BLE_ATT_ERR_UNLIKELY;
}
// set_preferred_mtu
static int gatt_svr_chr_ota_data_cb(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt,
                                    void *arg) {
  int rc;
  esp_err_t err;

  // store the received data into gatt_svr_chr_ota_data_val
  rc = gatt_svr_chr_write(ctxt->om, 1, sizeof(gatt_svr_chr_ota_data_val),
                          gatt_svr_chr_ota_data_val, NULL);

  // write the received packet to the partition
  if (updating) {
    err = esp_ota_write(update_handle, (const void *)gatt_svr_chr_ota_data_val,
                        packet_size);
    // if there is no error: increment packet count...
    
    if (err != ESP_OK) {
      // handle OTA write fail
      
      ESP_LOGE(LOG_TAG_GATT_SVR, "esp_ota_write failed (%s)!",
               esp_err_to_name(err));
    }

    num_pkgs_received++;
    ESP_LOGI(LOG_TAG_GATT_SVR, "Received packet %d", num_pkgs_received);
  }

  return rc;
}


// I want to copy this string into data_chunk
// "breaker_state: [0x05]
const char *data_chunk_str = 
"breaker_state: [0x05]\n"
"primary_handle_status: [0x03]\n"
"path_status: [0x01]\n"
"meter_update_no: [0x00]\n"
"period: [0x0000]\n"
"phase_A_line_frequency: [59.976852]\n"
"phase_A_rms_voltage: [2.280288]\n"
"phase_A_rms_current: [0.000000]\n"
"phase_A_power_factor: [1.000000]\n"
"phase_A_active_power: [0.000000]\n"
"phase_A_reactive_power: [0.000000]\n"
"phase_A_app_power: [0.000000]\n"
"phase_A_active_energy: [0.000000]\n"
"phase_A_reactive_energy: [0.000000]\n"
"phase_A_app_energy: [0.000000]\n"
"phase_A_reverse_active_energy: [0.000000]\n"
"phase_A_reverse_reactive_energy: [0.000000]\n"
"phase_A_reverse_app_energy: [0.000000]\n"
"breaker_state: [0x05]\n"
"---------------------------------------\n"
"phase_B_line_frequency: [59.976852]\n"
"phase_B_rms_voltage: [2.280288]\n"
"phase_B_rms_current: [0.000000]\n"
"phase_B_power_factor: [1.000000]\n"
"phase_B_active_power: [0.000000]\n"
"phase_B_reactive_power: [0.000000]\n"
"phase_B_app_power: [0.000000]\n"
"phase_B_active_energy: [0.000000]\n"
"phase_B_reactive_energy: [0.000000]\n"
"phase_B_app_energy: [0.000000]\n"
"phase_B_reverse_active_energy: [0.000000]\n"
"phase_B_reverse_reactive_energy: [0.000000]\n"
"phase_B_reverse_app_energy: [0.000000]\n";


void gatt_svr_init() {
  //just filling data, so it can be used by read_character callback.
  //FILL_ARRAY(data_chunk, DATA_SIZE);
  strncpy((char *)data_chunk, data_chunk_str, DATA_SIZE - 1);

  

  data_chunk[DATA_SIZE - 1] = '\0';
  ble_svc_gap_init();
  ble_svc_gatt_init();
  ble_gatts_count_cfg(gatt_svr_svcs);
  ble_gatts_add_svcs(gatt_svr_svcs);
}
