MiraOS for RuuviTag
===================

An example of running the MiraOS on RuuviTag hardware, to show how commissioning
and status can be made using NFC

Build and installation
----------------------

Get the tools required for nRF targets according to:
https://docs.lumenrad.io/miraos/2.8.0/description/toolchain/tools.html

The software is built to work with MiraOS version 2.2.2 or later.

Download and unpack mira to `src/vendor/libmira`. To get access to libmira contact sales@lumenradio.com.

Download and unpack nrf5-sdk, and place in `src/vendor/nrf5-sdk`. nrf5-sdk can be
[downloaded from Nordic Semiconductor](https://www.nordicsemi.com/Products/Development-software/nrf5-sdk).
Current supported version is 17.1.0. Note, the
original directory has a name similar to `nRF5_SDK_17.1.0_ddde560` and has to be
renamed.

```
make
```

Commissioning
------------

To commission the device, Use a phone and an NFC tag reader/writer app to write
an NDEF file containing following records:

1. Name
   - MIME: application/vnd.lumenradio.name
   - Payload: Name of the device, string up to 32 characters
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
   - Payload: 0 to 127 as integer value.

The current commissioning status can be read via NFC. The same set of records
is available, except for network key.

When commissioned, the current network status is also available.


Flashing
--------------------
First, add a license to the device on address 0x7F000 of length 0x1000, according to MiraOS documentation:

https://docs.lumenrad.io/miraos/2.8.0/description/licensing/licensing_tool.html#signing-a-connected-device

...or by flashing the license as a hex file if available.

Second, flash the application to the device:

```
nrfjprog -f nrf52 --program mira-demo-nrf52832ble-os.hex --sectorerase --verify
```

Decommissioning
--------------

To disable the node, use the same method as commissioning, but write:
- PAN-ID = ffffffff
- Encryption key = ffffffffffffffffffffffffffffffff
- Rate = 255
- Update interval = 65535

Local monitoring of sensor values
---------------------------------

When the device is commissioned, the sensor values is available via NFC as
fields in the NDEF records. The values is available as user-readable text
under the MIME type application/vnd.lumenradio.sensor.\*

Sensor values is updated at an interval specified via update interval
configuration

Packet format for data
-------------

The sensor sends an UDP packet at port 7338 to the root node of the network
containing:

- 16 bytes name of device, padded with zeroes. Note that a name with 16 bytes
  may not have a null-byte termination
- 40 bytes parent address
- list of sensor values, 9 bytes each
  - 1 byte type:
    - 0 = no type
    - 1 = Temperature
    - 2 = Pressure
    - 3 = Humidity
    - 4 = Battery
    - 5 = ETX
    - 6 = Clock drift
    - 7 = CO2
    - 8 = Acc_x
    - 9 = Acc_y
    - 10 = Acc_z
    - 11 = Move count
    - 12 = seq num
  - 4 bytes, MSB first, signed, sensor P value
  - 4 bytes, MSB first, unsigned, sensor Q value

The sensor values is transmitted as a rational value. To get the correct sensor
value, calculate P/Q.

This method makes it possible for the sensor to provide a proper range and
resolution for presentation.

With this format 11 sensor values can be added before fragmentation occurs.

Receiving and presenting
-----------------------

An example script for receiving sensor updates, and publish to InfluxDB is
provided as udp\_to\_influx.py

The script is designed to work together with the Mira border gateway

Disclaimer
----------

Copyright (c) 2018, LumenRadio AB All rights reserved.

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
