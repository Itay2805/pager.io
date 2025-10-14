import bleak
import asyncio
import uuid
import sys



PROTOCOL_CHAR = uuid.UUID(hex="170c3c75-fe39-b698-b744-6be096d7dc98")


async def handler(client: bleak.BleakClient):
    await client.write_gatt_char(PROTOCOL_CHAR, b'HELLO!')
    print(await client.read_gatt_char(PROTOCOL_CHAR))


async def main():
    if len(sys.argv) >= 2:
        async with bleak.BleakClient(sys.argv[1]) as client:
            await handler(client)

    else:
        # find the pager.io device
        scanner = bleak.BleakScanner()
        devices = [dev for dev in await scanner.discover() if dev.name == 'pager.io']
        assert len(devices) == 1, f"Too many pager.io devices: {devices}"
        device = devices[0]
        print(f"Found pager.io device: {device}")

        # setup a client
        async with bleak.BleakClient(device) as client:
            await handler(client)


if __name__ == '__main__':
    asyncio.run(main())
