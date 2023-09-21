# Commision
 1. Name
   - MIME: application/vnd.lumenradio.name
   - Payload: Name of the device, string up to 16 characters
2. Network PAN ID
   - MIME: application/vnd.lumenradio.net_panid
   - Payload: Hex, 8 characters / 4 bytes, MSB first: 13243546
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

# Decommission

To disable the node, use the same method as commissioning, but write:
- PAN-ID = ffffffff
- Encryption key = ffffffffffffffffffffffffffffffff
- Rate = 255
- Update interval = 65535

# Local monitoring of sensor values

When the device is commissioned, the sensor values is available via NFC as
fields in the NDEF records. The values is available as user-readable text
under the MIME type application/vnd.lumenradio.sensor.\*

Sensor values is updated at an interval specified via update interval
configuration

# Packet format for data

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
    - 6 = Acc_x
    - 7 = Acc_y
    - 8 = Acc_z
    - 9 = Move count
  - 4 bytes, MSB first, signed, sensor P value
  - 4 bytes, MSB first, unsigned, sensor Q value

The sensor values is transmitted as a rational value. To get the correct sensor
value, calculate P/Q.

This method makes it possible for the sensor to provide a proper range and
resolution for presentation.

With this format 10 sensor values can be added before fragmentation occurs.
