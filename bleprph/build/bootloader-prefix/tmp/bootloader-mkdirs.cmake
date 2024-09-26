# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Espressif5.3/frameworks/esp-idf-v5.3.1/components/bootloader/subproject"
  "C:/Users/E0739939/Documents/__BLE_OTA/ble_ota-main/ble_ota-main/bleprph/build/bootloader"
  "C:/Users/E0739939/Documents/__BLE_OTA/ble_ota-main/ble_ota-main/bleprph/build/bootloader-prefix"
  "C:/Users/E0739939/Documents/__BLE_OTA/ble_ota-main/ble_ota-main/bleprph/build/bootloader-prefix/tmp"
  "C:/Users/E0739939/Documents/__BLE_OTA/ble_ota-main/ble_ota-main/bleprph/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/E0739939/Documents/__BLE_OTA/ble_ota-main/ble_ota-main/bleprph/build/bootloader-prefix/src"
  "C:/Users/E0739939/Documents/__BLE_OTA/ble_ota-main/ble_ota-main/bleprph/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/E0739939/Documents/__BLE_OTA/ble_ota-main/ble_ota-main/bleprph/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/E0739939/Documents/__BLE_OTA/ble_ota-main/ble_ota-main/bleprph/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
