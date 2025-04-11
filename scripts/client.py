import bleak
import asyncio


async def main():
    # find the pager.io device
    scanner = bleak.BleakScanner()
    devices = [dev for dev in await scanner.discover() if dev.name == 'pager.io']
    assert len(devices) == 1, f"Too many pager.io devices: {devices}"

    # setup a client
    client = bleak.BleakClient(devices[0])
    assert await client.connect(), "Failed to connect"
    try:
        pass
    finally:
        # disconnect from the client
        await client.disconnect()


if __name__ == '__main__':
    asyncio.run(main())
