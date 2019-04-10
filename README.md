MiraOS for RuuviTag
===================

An example of running the MiraOS on RuuviTag hardware, to show how commissioning
and status can be made using NFC

Build and installation
----------------------

Setup the toolchain according to:
https://docs.lumenrad.io/miraos/toolchain/

The software is built to work with MiraOS version 2.2.2 or later.

Update LIBDIR in Makefile to point to the unpacked version of libmira, type:

```
make
```

Connect the RuuviTag to the computer, either on the development board, or via a
J-Link, and type:

```
make flashall
```

The tag should then be programmed

Commissioning
------------

To commission the device, Use a phone and an NFC tag reader/writer app to write
an NDEF file containing following records:

1. Name
   - MIME: application/vnd.lumenradio.name
   - Payload: Name of the device, string up to 32 characters
2. Network PAN ID
   - MIME: application/vnd.lumenradio.panid
   - Payload: Hex, 8 characters / 4 bytes, MSB first
3. Network encryption key
   - MIME: application/vnd.lumenradio.net_key
   - Payload: Hex string, 32 characters
4. Node data rate (see mira_net_config_t in documentation)
   - MIME: application/vnd.lumenradio.net_rate
   - Payload: Hex string, 2 characters / 1 byte
5. Update interval - seconds between measurement
   - MIME: application/vnd.lumenradio.update_interval
   - Payload: Hex string, 4 characters / 2 bytes, MSB first

The current commissioning status can be read via NFC. The same set of records
is available, except for network key.

When commissioned, the current network status is also available.

Decommissioning
--------------

To disable the node, use the same method as commissioning, but write:
- PAN-ID = ffffffff
- Encryption key = ffffffffffffffffffffffffffffffff
- Rate = ff
- Update interval = ffff

Local monitoring of sensor values
---------------------------------

When the device is commissioned, the sensor values is available via NFC as
fields in the NDEF records. The values is available as user-readable text
under the MIME type application/vnd.lumenradio.sensor.\*

Sensor values is updated at an interval specified via update interval
configuration

Packet format
-------------

The sensor sends an UDP packet at port 7337 to the root node of the network
containing:

- 32 bytes name of device, padded with zeroes. Note that a name with 32 bytes
  may not have a null-byte termination
- list of sensor values, 9 bytes each
  - 1 byte type:
    - 0 = no type
    - 1 = Temperature
    - 2 = Pressure
    - 4 = Humidity
    - 5 = Battery
    - 6 = ETX
    - 7 = Clock drift
  - 4 bytes, MSB first, signed, sensor P value
  - 4 bytes, MSB first, unsigned, sensor Q value

The sensor values is transmitted as a rational value. To get the correct sensor
value, calculate P/Q

This method makes it possible for the sensor to provide a proper range and
resolution for presentation

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
