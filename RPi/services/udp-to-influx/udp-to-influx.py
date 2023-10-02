#!/usr/bin/env python

import socket
import influxdb
import datetime
import math

_units = ['none', 'deg C', 'Pa', '%', 'V', '', 'mg', 'mg', 'mg', '', '']
_types = ['none', 'temperature', 'pressure', 'humidity', 'battery', 'etx', 'acc_x', 'acc_y', 'acc_z', 'move_count']
root_addr = "fd00::b0e9:c734:1806:20d1"
pdr_dict = { root_addr:0 }
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
            if (parent_address == None or nbr_of_hops > 30):
                # Safety if stuck in loop or parent is not present
                return -1

        return nbr_of_hops

# Value processing

class SampleValue:

    def __init__(self, raw):
        self.type = raw[0]
        value_p = sum([v<<(8*(3-i)) for i,v in enumerate(raw[1:5])])
        if value_p & 0x80000000:
            value_p -= 0x100000000
        value_q = sum([v<<(8*(3-i)) for i,v in enumerate(raw[5:9])])
        if value_q != 0:
            self.value = float(value_p) / value_q
        else:
            self.value = float(value_p)

    def __str__(self):
        if (self.type < len(_types)):
            return f"{_types[self.type]} = {self.value} {_units[self.type]}"
        else:
            return ""

class TagData:
    def __init__(self, raw):
        self.name = (raw[0:16]).decode('utf-8')
        self.parent = (raw[16:56]).decode('utf-8')
        self.seq_no = int.from_bytes(raw[56:60], 'big')
        self.sensors = []
        for sensor_start in range(60,len(raw),9):
            self.sensors.append(SampleValue(raw[sensor_start:sensor_start+9]))

    def __str__(self):
        sensor_str = " ".join([str(sensor) for sensor in self.sensors])
        return f"{self.name}: parent = {self.parent} seq_no = {self.seq_no} {sensor_str}"

# Packet receiver

def udp_server(host='::', port=7338):
    s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    s.bind((host, port))
    while True:
        (data, (host, port, flowinfo, scopeid)) = s.recvfrom(128*1024)
        yield (TagData(bytearray(data)), host, port)

def calculate_pdr(host, seq_no):
    global pdr_dict

    #Calculate the PDR
    if host in pdr_dict.keys():
        if seq_no == 0:
            pdr_dict[host] = 0
        else :
            pdr_dict[host] = pdr_dict[host] + 1
    else:
        if seq_no > 0:
            pdr_dict[host] = seq_no
        else:
            pdr_dict[host] = 0

    if seq_no == 0:
        pdr = 1.0
    else :
        pdr = float(pdr_dict[host]) / seq_no

    if pdr > 1:
        pdr_dict[host] = seq_no
        pdr = 1.0

    #print ("PDR: "+ pdr + "\n")
    return pdr * 100

# InfluxDB reporter

class DBReporter:

    def __init__(self, host, port, user, password, dbname, root):
        self.client = influxdb.InfluxDBClient(host, port, user, password, dbname)
        self.calc_hops = CalculateNbrOfHops(root)

    def upload(self, tagdata, host):
        hops = self.calc_hops.calc_hops(host, tagdata.parent)
        now = datetime.datetime.utcnow().isoformat() + 'Z'
        body = []
        acc_avg = 0

        for sensor in tagdata.sensors:
	    #print("DEBUG: TYPE:", sensor.type)
            if (sensor.type < len(_types)):
                if _types[sensor.type] == 'etx':
                    fields = {
                        "etx": sensor.value,
                        "neigbour": tagdata.parent,
                        "hops": hops
                   }
                else:
                    fields = {
                        "value": sensor.value,
                    }
                if (_types[sensor.type] == 'acc_x') or (_types[sensor.type] == 'acc_y') or (_types[sensor.type] == 'acc_z'):
                    acc_avg += sensor.value * sensor.value
                body.append({
                    "measurement": _types[sensor.type],
                    "tags": {
                        "sensor": tagdata.name,
                        "address": host,
                    },
                    "time": now,
                    "fields": fields
                })
            else:
                print("IndexError: skipping this value")

        # Add pdr
        pdr = calculate_pdr(host, tagdata.seq_no)
        body.append({
            "measurement": 'seq_no',
            "tags": {
                "sensor": tagdata.name,
                "address": host,
            },
            "time": now,
            "fields": {
                "pdr": pdr,
             }
        })

        # Add acc_rms
        acc_rms = math.sqrt(acc_avg)
        body.append({
            "measurement": 'acc_rms',
            "tags": {
                "sensor": tagdata.name,
                "address": host,
            },
            "time": now,
            "fields": {
                "value": acc_rms
            }
        })
        self.client.write_points(body)
        now = datetime.datetime.now()
        print("[{}] host: {}, hops: {}, sensor: {}".format( now.strftime("%H:%M:%S"), host, hops, tagdata))


# Main
db = DBReporter('localhost', 8086, 'mirauser', 'mirapassword', 'miradb', root_addr)

for tagdata, host, port in udp_server():
    db.upload(tagdata, host)

