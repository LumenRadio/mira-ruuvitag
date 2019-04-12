#!/usr/bin/env python

import socket
import influxdb
import datetime


# Calculate number of hops towards root.

class CalculateNbrOfHops:
    def __init__(self, root):
        self.dict_address_parent = {}
        self.root_address = root

    def calc_hops(self, host, parent_address):
        nbr_of_hops = 1
        # key is the host, value is the parent
        self.dict_address_parent[host] = parent_address
        # Will add if the host if not present, otherwise update

        while (parent_address != self.root_address):
            nbr_of_hops = nbr_of_hops + 1

            parent_address = self.dict_address_parent.get(parent_address)
            if (parent_address == None or nbr_of_hops > 50):
                return -1
                # Saftey if stuck in loop or parent not present

        return nbr_of_hops

# Value processing

class SampleValue:
    units = ['none', 'deg C', 'Pa', '%', 'V', '', '']
    types = ['none', 'temperature', 'pressure', 'humidity', 'battery', 'etx', 'clock_drift']

    def __init__(self, raw):
    #    self.name = str(raw[0:15]).strip('\0')
    #    self.unit = raw[15]
        self.type = raw[0]
        value_p = sum([v<<(8*(3-i)) for i,v in enumerate(raw[1:5])])
        if value_p & 0x80000000:
            value_p -= 0x100000000
        value_q = sum([v<<(8*(3-i)) for i,v in enumerate(raw[5:9])])
        self.value = float(value_p) / value_q

    def __str__(self):
    #    return "%s = %f %s" % (self.name, self.value, self.units[self.unit])
        return "%s = %f %s" % (self.types[self.type], self.value, self.units[self.type])

class TagData:
    def __init__(self, raw):
        self.name = str(raw[0:32]).strip('\0')
        self.parent = str(raw[32:72]).strip('\0')
        self.sensors = []
        for sensor_start in range(72,len(raw),9):
            self.sensors.append(SampleValue(raw[sensor_start:sensor_start+9]))

    def __str__(self):
        return self.name + ": parent = " + self.parent + " " + " ".join(str(sensor) for sensor in self.sensors)

# Packet receiver

def udp_server(host='::', port=7338):
    s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    s.bind((host, port))
    while True:
        (data, (host, port, flowinfo, scopeid)) = s.recvfrom(128*1024)
        yield (TagData(bytearray(data)), host, port)

# InfluxDB reporter

class DBReporter:
    types = ['none', 'temperature', 'pressure', 'humidity', 'battery', 'etx', 'clock_drift']

    def __init__(self, host, port, user, password, dbname, root):
        self.client = influxdb.InfluxDBClient(host, port, user, password, dbname)
        self.calc_hops = CalculateNbrOfHops(root)

    def upload(self, tagdata):
        now = datetime.datetime.utcnow().isoformat() + 'Z'
        body = []
        for sensor in tagdata.sensors:

            if sensor.types[sensor.type] == 'etx':
                fields = {
                    "etx": sensor.value,
                    "neigbour": tagdata.parent,
                    "hops": self.calc_hops.calc_hops(host, tagdata.parent)
                }
            else:
                fields = {
                    "value": sensor.value,
                }
            body.append({
                "measurement": self.types[sensor.type],
                "tags": {
                    "sensor": tagdata.name,
                    "address": host,
                },
                "time": now,
                "fields": fields
            })
        self.client.write_points(body)


# Main

db = DBReporter('localhost', 8086, 'monitoring', 'uploadstuff', 'monitoring', "fd00::75c4:e996:a200:cf7b")

for tagdata, host, port in udp_server():
    now = datetime.datetime.now()
    print "[", now.strftime("%H:%M:%S"), "] host:", host, "sensor:", tagdata
    db.upload(tagdata)
