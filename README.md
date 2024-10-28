MiraOS for RuuviTag
===================
This repo provides a guide for setting up a sensor network using Mira products and RuuviTags. Sensor data is stored in InfluxDB and displayed via a Grafana dashboard.

# Overview
RuuviTags form a Mira mesh network, transmitting sensor data to a border gateway. The gateway is a Raspberry Pi equipped with a MiraUSB, which serves as the network's root and acts as a Radio on a Stick (RoaS). The mira-gateway systemd service manages communication with the MiraUSB and directs traffic into the host's network stack. On the host, three containerized services operate:

1. InfluxDB: A database for sensor data storage.
2. udp-to-influx: Processes packets from the Mira gateway and saves the sensor data to InfluxDB.
3. Grafana: Retrieves and visually represents data in a dashboard.

# Quick start

The following components are needed.

1. libmira, version 2.10.0 or higher
2. MiraUSB
3. Raspberry Pi 3, 4 or 5 with a SD card and a power supply
4. A couple of Ruuvi tags
5. Mira licenses for the Ruuvi tags

To get any of the Mira products, please get in contact with sales@lumenradio.com.
## Setup raspbian

1. Follow the instructions on https://www.raspberrypi.com/software/ to create a new installation of Raspebrry Pi OS 64 bit.
2. Connect your raspberry Pi to the internet. 
3. Create a sudo user and log in. 

> [!NOTE]
> 64 bit OS is required.

## Install docker
```
curl -sSL https://get.docker.com | sh
sudo usermod -aG docker <username>
```

## Set up influxdb, grafana, udp-to-influx and mira-gateway services
On the Raspberry Pi
Run:
```
cd ~
git clone https://github.com/LumenRadio/mira-ruuvitag.git
```

Download the latest Mira gateway .deb package installer. It can be found on https://dl.lumenradio.com/mira/. Contact sales@lumenradio.com for more information.

Place the mira-gateway and mira-monitoring .deb package installers for your architecture (arm64 or armhf) in mira-ruuvitag/RPi. Update arch variable in mira-ruuvitag/RPi/install_dashboard.sh if needed.

> [!NOTE]
> These .deb installers are only available from Mira 2.10.0 and above, and only supports raspberry pi targets.

Connect the MiraUSB to the raspberry pi.

Run:
```
cd mira-ruuvitag/RPi
sudo ./install_dashboard.sh
```

This will create an influx database where the sensor data will be stored, routed by udp_to_influx2.py. It also start the mira-monitoring services, which serves an API for network monitoring data at <raspberry_pi_ip>:3001, which in turn is used by the grafana infinity plugin to display network health data in the dashboard.

After some time, you should be able to access grafana at <raspberry_pi_ip>:3000 in your browser. Credentials are u: mirademo, p: demomira.

> [!NOTE] 
> The grafana docker container will install all external plugins on every reboot. This will fail if there is no internet connection, which will make grafana not start. To enable offline start of grafana, comment out GF_INSTALL_PLUGINS from the docker file and run "sudo docker compose down" followed by "sudo docker compose up". Docker compose must run at least once with GF_INSTALL_PLUGINS present in the compose file and with an internet connection to download the plugin to persistent storage before it can be commented out.

## Setting up RuuviTags

Get the tools required for nRF targets according to:
https://docs.lumenrad.io/miraos/2.8.0/description/toolchain/tools.html

The software is built to work with MiraOS version 2.10.0 or later.

Download and unpack mira to `[src vendor libmira](src/vendor/libmira)`. To get access to libmira contact sales@lumenradio.com.

Download and unpack nrf5-sdk, and place in `src/vendor/nrf5-sdk`. nrf5-sdk can be
[downloaded from Nordic Semiconductor](https://www.nordicsemi.com/Products/Development-software/nrf5-sdk).
As of libmira 2.8.0, the supported version is 17.1.0. Note, the
original directory has a name similar to `nRF5_SDK_17.1.0_ddde560` and has to be
renamed.

## Bulid
Stand in the top level directory and run
```
make
```
## Flash

First, add a license to the device on address 0x7F000 of length 0x1000, according to MiraOS documentation:

https://docs.lumenrad.io/miraos/2.8.0/description/licensing/licensing_tool.html#signing-a-device-with-remotely-generated-license

...or by flashing the license as a hex file if available.

Second, flash the application to the device:

```
nrfjprog -f nrf52 --program mira-ruuvitag-nrf52832ble-os.hex --sectorerase --verify
```

## Commission

To commission the devices, Use a phone and an NFC tag reader/writer app to write
an NDEF file containing following records:

1. Name
   - MIME: application/vnd.lumenradio.name
   - Payload: Name of the device, string up to 16 characters. Should be unique for each Ruuvi-tag
2. Network PAN ID
   - MIME: application/vnd.lumenradio.net_panid
   - Payload: Hex, 8 characters / 4 bytes, MSB first
3. Network encryption key
   - MIME: application/vnd.lumenradio.net_key
   - Payload: Hex string, 32 characters
4. Node data rate (see mira_net_config_t in documentation)
   - MIME: application/vnd.lumenradio.net_rate
   - Payload: 0 to 10 as an integer value.
5. Update interval - seconds between measurement
   - MIME: application/vnd.lumenradio.update_interval
   - Payload: 0 to 65534 as an integer value.
6. Move threshold - Threshold value above which motion should be detected
   - MIME: application/vnd.lumenradio.move_theshold
   - Payload: 0 to 127 as integer value

When commisioning several Ruuvi tags, make sure to set a different name in step 1. All other parameters can remain the same.

The encryption key and Network PAN ID needs to be the same as used by the gateway. For more information about configuring the gateway, see https://docs.lumenrad.io/miraos/2.8.1/description/gateway.html.

The current commissioning status can be read via NFC. 
A commissioned node will report its network status. It should go from "not associated" to "associated" and then finally "joined" indicating that the node has sucessfully joined the network.

## Look at the dashboard

You should now be able to see the grafana dashboard at <raspberry_pi_ip>:3000. The dashboard contains the different Ruuvi tag sensors data and some network information from Mira such as:
* Mesh topology.
* Packet delivery rate from tag to root as well as to the immediate parent.
* RSSI between each link.
* Packets sent/received statistics.
* TX/RX utilization of each node.

Disclaimer
----------

Copyright (c) 2024, LumenRadio AB All rights reserved.

The software is provided as an example of how to integrate MiraOS with a
RuuviTag. The software is delivered without guarantees, and is not affiliated
with Ruuvi in any way.

No guarantees is taken for protocol stability of the software, and future
updates may change API.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
