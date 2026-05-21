# Contents
This repo contains the code for the 2025/26 OUR VCU.

# Client 
This directory deals with the client side, for live-telemetry and remote bootloading from a laptop. This is primarily written in python. To use, install Astral's UV and run `uv sync`.

# ESP
This directory contains the code for the ESP32-C. Clone and install the repo for [idf.py](https://github.com/espressif/esp-idf). We use this to build our project. Activate idf.py by running `source export.sh` - this file is found in the repo that you just downloaded. 

Run `idf.py menuconfig` and activate Component config -> HTTP Server -> WebSocket servedr support. We are using an ESP32-3C, so run `idf.py set-target esp32c3` so that we build for the correct chip architecture. Run `idf.py fullclean` to remove anything created by the build system in case of screw up. To build, we run `idf.py build` inside the esp/ directory. To flash to an ESP that is connected via USB we run `idf.py flash monitor` to flash the build onto the EPS32 and then open up the console. Running `idf.py -p <port ESP is using> flash monitor` to disambiguate the ESP. To exit the monitor enter `Ctrl + ]`. 