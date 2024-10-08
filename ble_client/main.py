import asyncio
import datetime
from bleak import BleakClient, BleakScanner


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

#safety Service UUID
SAFETY_SERVICE_UUID = '6561746F-6E62-6C65-7365-727669636501'
#safety Service Characteristic Request UUID
SAFETY_CHARACTERISTIC_REQUEST_UUID = '6561746F-6E62-6C65-7365-727669636502'
#safety Service Characteristic Response UUID
SAFETY_CHARACTERISTIC_RESONSE_UUID = '6561746F-6E62-6C65-7365-727669636503'



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
            print("Reading Device Info...")
            # Print the services and characteristics offered
            services = await client.get_services()
            for service in services:
                print("Service UUID:", service.uuid)
                for characteristic in service.characteristics:
                    print("Characteristic UUID:", characteristic.uuid)
            
            # Define the handlers
            

            # Subscribe to Device Info service and characteristics
            #await client.start_notify(DEVICE_INFO_SERVICE_UUID, _device_info_notification_handler)
            manufacture_id = await client.read_gatt_char(DEVICE_INFO_MANUFACTURE_CHR_UUID)
            #print model number
            print("-------------------------------")
            print("Manufacturer: ", manufacture_id.decode())
            print("-------------------------------")
            model_number = await client.read_gatt_char(DEVICE_INFO_MODEL_CHR_UUID)
            
            #print model number
            print("-------------------------------")
            print("Model Number: ", model_number.decode())
            print("-------------------------------")

            # Wait for notifications (adjust the sleep time as needed)
            await asyncio.sleep(5)

            # Stop notifications
            #await client.stop_notify(DEVICE_INFO_MANUFACTURE_CHR_UUID)
            #await client.stop_notify(DEVICE_INFO_MODEL_CHR_UUID)

    except Exception as e:
        print(f"An error occurred while reading device info: {e}")


def main():
    # print to choose two options. Either read telemetry or send OTA
    print("Choose an option:")
    print("1. Read Device Info")
    print("2. Send OTA")
    option = input("Enter your choice: ")
    # if send OTA, then run the send_ota function
    if option == "2":
        asyncio.run(send_ota("ota_ble_FW.bin"))
    # if read telemetry, then run the read_telemetry function
    elif option == "1":
        print("Reading Device Info")
        asyncio.run(read_device_info())
        # jump to choose option again
        asyncio.run(main())


if __name__ == '__main__':
    main()
        
        
