'''
script to send the firmware .bin file to the ESP over websocket
'''

import asyncio
import websockets
import time

esp_address = 'ws://192.168.4.1/ws'
CHUNK_SIZE = 256

async def init_transfer(ws):
	print("Initialising data transfer")
	await ws.send("START")

async def end_transfer(ws):
	print("Ending data transfer")
	await ws.send("END")

async def send_chunk(ws, chunk):
	await ws.send(chunk)
    
async def send_binary_chunks(ws,fpath):
	with open(fpath,"rb") as f:
		await init_transfer(ws)
		time.sleep(1) # NOTE: blocks event loop
          
		while True:
			chunk = f.read(CHUNK_SIZE)
			if not chunk:
				break

			else:
				await send_chunk(ws,chunk)

		time.sleep(1) # NOTE: blocks event loop
		await end_transfer(ws)


async def receiver(ws):
    async for msg in ws:
        print(f"received: {msg}")

async def connect():
	async with websockets.connect(esp_address) as websocket:
		print("###################### CONNECTED ######################")
		send_task = send_binary_chunks(ws=websocket, fpath="our.bin")
		recv_task = asyncio.create_task(receiver(websocket))
		await asyncio.gather(send_task,recv_task)

if __name__ == "__main__":
    asyncio.run(connect())