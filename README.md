# Intro
This is a simpel OTA example where we can send a new firmware to the device and the device will update itself.
This involves using Blutooth hardware of the PC or laptop to send the new firmware to the device.
The device FW is preent in `bleprph` folder and the PC FW is present in `ble_client` folder.

# Folder Sructure
- bleprph: This folder contains the device FW.
- ble_client: This folder contains the PC FW.
```
  .\bleprph                              .\ble_client
    ├── .\main                               ├── main.py
            ├── CMakeLists.txt               ├── ota_ble_FW.bin
            ├── gap.c                       
            ├── gap.h                       
            ├── gatt_svr.c                  
            ├── gatt_svr.h                   
            └── main.c                       
    ├── CMakeLists.txt                      
                            
                            
```

# How does it work?
1. First build the FW. The code in `bleprph` folder is based on `ESP-IDF 5.3.2` . So, you need to have ESP-IDF installed on your system.
2. Once the FW is built, you will get a files in `bleprph/build` folder. Flash it on an ESP32 device.
3. Now, run the `main.py` file present in `ble_client` folder. This will start the BLE client.

# How to send the new FW?
running main.py will give the console output as below:
```
$ python main.py
Connected to device: 24:0A:C4:00:00:00
OTA Service discovered
OTA Characteristic discovered
Sending the new firmware
```
This means the BLE client is connected to the device and the OTA service and characteristic are discovered.
Now, the BLE client will send the new FW to the device. The device will update itself with the new FW.
