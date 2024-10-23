import asyncio
import datetime
from bleak import BleakClient, BleakScanner

import proto.device_feature_pb2 as DeviceFeature

OTA_DATA_UUID = '23408888-1F40-4CD8-9B89-CA8D45F8A5B0'
OTA_CONTROL_UUID = '7AD671AA-21C0-46A4-B722-270E3AE3D830'

SVR_CHR_OTA_CONTROL_NOP = bytearray.fromhex("00")
SVR_CHR_OTA_CONTROL_REQUEST = bytearray.fromhex("01")
SVR_CHR_OTA_CONTROL_REQUEST_ACK = bytearray.fromhex("02")
SVR_CHR_OTA_CONTROL_REQUEST_NAK = bytearray.fromhex("03")
SVR_CHR_OTA_CONTROL_DONE = bytearray.fromhex("04")
SVR_CHR_OTA_CONTROL_DONE_ACK = bytearray.fromhex("05")
SVR_CHR_OTA_CONTROL_DONE_NAK = bytearray.fromhex("06")


# defining Breaker related UUIDs

# Device Info service 
#define GATT_MANUFACTURER_NAME_UUID 0x2A29
# chagne endianness of the data 
DEVICE_INFO_SERVICE_UUID = '0000180a-0000-1000-8000-00805f9b34fb'#[::-1] #bytearray.fromhex("0A18")
DEVICE_INFO_MANUFACTURE_CHR_UUID = '00002a29-0000-1000-8000-00805f9b34fb'#[::-1] #bytearray.fromhex("292A")
# 16 bit uuid for Device Info
DEVICE_INFO_MODEL_CHR_UUID = '00002a24-0000-1000-8000-00805f9b34fb'#[::-1] #bytearray.fromhex("242A")



# defining 522 bytes service
SERVICE_512_BYTES_UUID = '0000180d-0000-1000-8000-00805f9b34fb'
#512 bytes Service Characteristic Request UUID
CHARACTERISTIC_512_BYTES_UUID = '00002a37-0000-1000-8000-00805f9b34fb'


#safety Service UUID
SAFETY_SERVICE_UUID = '6561746F-6E62-6C65-7365-727669636501'
#safety Service Characteristic Request UUID
SAFETY_CHARACTERISTIC_REQUEST_UUID = '6561746F-6E62-6C65-7365-727669636502'
#safety Service Characteristic Response UUID
SAFETY_CHARACTERISTIC_RESONSE_UUID = '6561746F-6E62-6C65-7365-727669636503'

#device features UUID
GATT_DEVICE_FEATURES_UUID = '0000180f-0000-1000-8000-00805f9b34fb'
GATT_DEVICE_TYPE_UUID = '00002a3c-0000-1000-8000-00805f9b34fb'
GATT_VOLTAGE_RATING_UUID = '0000226c-0000-1000-8000-00805f9b34fb'
GATT_CURRENT_RATING_UUID = '00002a69-0000-1000-8000-00805f9b34fb'

async def _search_for_esp32():
    print("Searching for ESP32...")
    esp32 = None

    devices = await BleakScanner.discover()
    for device in devices:
        if device.name == "esp32":
            esp32 = device

    if esp32 is not None:
        print("ESP32 found!")
    else:
        print("ESP32 has not been found.")
        assert esp32 is not None

    return esp32


async def send_ota(file_path):
    t0 = datetime.datetime.now()
    queue = asyncio.Queue()
    firmware = []

    esp32 = await _search_for_esp32()
    async with BleakClient(esp32) as client:

        async def _ota_notification_handler(sender: int, data: bytearray):
            """
            Handles OTA notifications received from the server.

            Parameters:
            - sender (int): The sender of the notification.
            - data (bytearray): The data received in the notification.

            Returns:
            None
            """
            if data == SVR_CHR_OTA_CONTROL_REQUEST_ACK:
                print("ESP32: OTA request acknowledged.")
                await queue.put("ack")
            elif data == SVR_CHR_OTA_CONTROL_REQUEST_NAK:
                print("ESP32: OTA request NOT acknowledged.")
                await queue.put("nak")
                await client.stop_notify(OTA_CONTROL_UUID)
            elif data == SVR_CHR_OTA_CONTROL_DONE_ACK:
                print("ESP32: OTA done acknowledged.")
                await queue.put("ack")
                await client.stop_notify(OTA_CONTROL_UUID)
            elif data == SVR_CHR_OTA_CONTROL_DONE_NAK:
                print("ESP32: OTA done NOT acknowledged.")
                await queue.put("nak")
                await client.stop_notify(OTA_CONTROL_UUID)
            else:
                print(f"Notification received: sender: {sender}, data: {data}")

        # subscribe to OTA control
        await client.start_notify(
            OTA_CONTROL_UUID,
            _ota_notification_handler
        )

        # compute the packet size
         # 253
        print("Client MTU size: ", client.mtu_size)
        packet_size = (client.mtu_size - 3)

        # write the packet size to OTA Data
        print(f"Sending packet size: {packet_size}.")
        await client.write_gatt_char(
            OTA_DATA_UUID,
            packet_size.to_bytes(2, 'little'),
            response=True
        )

        # split the firmware into packets
        with open(file_path, "rb") as file:
            while chunk := file.read(packet_size):
                firmware.append(chunk)

        # write the request OP code to OTA Control
        print("Sending OTA request.")
        await client.write_gatt_char(
            OTA_CONTROL_UUID,
            SVR_CHR_OTA_CONTROL_REQUEST
        )

        # wait for the response
        await asyncio.sleep(1)
        if await queue.get() == "ack":
            # sequentially write all packets to OTA data
            for i, pkg in enumerate(firmware):
                print(f"Sending packet {i+1}/{len(firmware)}.")
                await client.write_gatt_char(
                    OTA_DATA_UUID,
                    pkg,
                    response=True
                )

            # write done OP code to OTA Control
            print("Sending OTA done.")
            await client.write_gatt_char(
                OTA_CONTROL_UUID,
                SVR_CHR_OTA_CONTROL_DONE
            )

            # wait for the response
            await asyncio.sleep(1)
            if await queue.get() == "ack":
                dt = datetime.datetime.now() - t0
                print(f"OTA successful! Total time: {dt}")
            else:
                print("OTA failed.")

        else:
            print("ESP32 did not acknowledge the OTA request.")


async def read_device_info():
    try:
        esp32 = await _search_for_esp32()
        
        if esp32 is None:
            print("ESP32 not found.")
            return

        async with BleakClient(esp32) as client:
            print("Client MTU size: ", client.mtu_size)
            print("Reading Device Info...")
            # Print the services and characteristics offered
            services = await client.get_services()
            for service in services:
                print("Service UUID:", service.uuid)
                for characteristic in service.characteristics:
                    print("     Characteristic UUID:", characteristic.uuid)
            
            # Define the handlers
            

            # Subscribe to Device Info service and characteristics
            #await client.start_notify(DEVICE_INFO_SERVICE_UUID, _device_info_notification_handler)
            manufacture_id = await client.read_gatt_char(DEVICE_INFO_MANUFACTURE_CHR_UUID)
            #print model number
            print("---------------------------------")
            print("Manufacturer: ", manufacture_id.decode())
            print("---------------------------------")
            model_number = await client.read_gatt_char(DEVICE_INFO_MODEL_CHR_UUID)
            
            #print model number
            print("---------------------------------")
            print("Model Number: ", model_number.decode())
            print("---------------------------------")

            # Wait for notifications (adjust the sleep time as needed)
            await asyncio.sleep(5)

            # Stop notifications
            #await client.stop_notify(DEVICE_INFO_MANUFACTURE_CHR_UUID)
            #await client.stop_notify(DEVICE_INFO_MODEL_CHR_UUID)

    except Exception as e:
        print(f"An error occurred while reading device info: {e}")



async def read_telemtry_data():
    try:
        esp32 = await _search_for_esp32()
        
        if esp32 is None:
            print("ESP32 not found.")
            return

        async with BleakClient(esp32) as client:
            try:
                print("Reading Telemetry Data from the new service...")

                # Read the characteristic in chunks if necessary
                data = bytearray()
                chunk_size = 2500
                offset = 0

                while len(data) < chunk_size:
                    chunk = await client.read_gatt_char(CHARACTERISTIC_512_BYTES_UUID, offset=offset)
                    data.extend(chunk)
                    offset += len(chunk)
                    if len(chunk) == 0:
                        break  # No more data to read
            except Exception as e:
                print(f"An error occurred while reading Telemetry data: {e}")
                return
        string_data = data.decode('ascii') 
        print("|----Telemetry Data----|")
        print(string_data)

    except Exception as e:
        print(f"An error occurred while reading Telemetry data: {e}")

# read device features
async def read_device_features():
    try:
        esp32 = await _search_for_esp32()
        
        if esp32 is None:
            print("ESP32 not found.")
            return

        async with BleakClient(esp32) as client:
            print("Reading Device Features...")
            # Print the services and characteristics offered
            # Subscribe to Device Info service and characteristics
            #await client.start_notify(DEVICE_INFO_SERVICE_UUID, _device_info_notification_handler)
            device_feature = DeviceFeature
            #device_type = await client.read_gatt_char(GATT_DEVICE_TYPE_UUID)
            #voltage_rating = await client.read_gatt_char(GATT_VOLTAGE_RATING_UUID)
            #current_rating = await client.read_gatt_char(GATT_CURRENT_RATING_UUID)
            device_feature_data = await client.read_gatt_char(GATT_DEVICE_TYPE_UUID)
            device_feature = DeviceFeature.Device_Feature()
            device_feature.ParseFromString(device_feature_data)
            print("whole data")
            print(device_feature)
            print("Device Type")
            print(device_feature.device_type)
            print("Voltage Rating")
            print(device_feature.voltage_rating)

            
            
            ## deserialize the data


            #


            #device_feature.voltage_rating = int.from_bytes(voltage_rating, byteorder='little')
            #device_feature.current_rating = int.from_bytes(current_rating, byteorder='little')

            ## Use the unpacked message
            #print(f"Device Type: {device_feature.device_type}")
            #print(f"Voltage Rating: {device_feature.voltage_rating}")
            #print(f"Current Rating: {device_feature.current_rating}")
            ## Wait for notifications (adjust the sleep time as needed)
            await asyncio.sleep(5)

            # Stop notifications
            #await client.stop_notify(DEVICE_INFO_MANUFACTURE_CHR_UUID)
            #await client.stop_notify(DEVICE_INFO_MODEL_CHR_UUID)

    except Exception as e:
        print(f"An error occurred while reading device features: {e}")



def main():
    # print to choose two options. Either read telemetry or send OTA
    print("Choose an option:")
    print("1. Read Device Info")
    print("2. Send OTA")
    print("3. Read Telemetry Data")
    print("4. Read Device Features")
    option = input("Enter your choice: ")
    # if send OTA, then run the send_ota function
    if option == "2":
        asyncio.run(send_ota("ota_ble_FW.bin"))
    # if read telemetry, then run the read_telemetry function
    elif option == "1":
        print("Reading Device Info")
        asyncio.run(read_device_info())
        # jump to choose option again
        #asyncio.run(main())

    elif option == "3":
        print("Reading Telemetry Data...")
        asyncio.run(read_telemtry_data())

    elif option == "4":
        print("Reading Device Features...")
        asyncio.run(read_device_features())
    

if __name__ == '__main__':
    main()
        
        
