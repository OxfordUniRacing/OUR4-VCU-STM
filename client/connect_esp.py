'''
open a web socket connection to the ESP32
'''

import asyncio
import websockets

esp_address = 'ws://192.168.4.1/ws'

async def sender(ws):
    await ws.send('this is a test message...')

async def receiver(ws):
    async for msg in ws:
        print(f"received: {msg}")

async def connect():
    async with websockets.connect(esp_address) as websocket:
        print("###################### CONNECTED ######################")
        send_task = asyncio.create_task(sender(websocket))
        recv_task = asyncio.create_task(receiver(websocket))
        await asyncio.gather(send_task, recv_task)  # wait for both to finish

if __name__ == "__main__":
    asyncio.run(connect())