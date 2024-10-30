#!/usr/bin/env python

import os
import socket
import ipaddress
from influxdb_client import InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS
import datetime
import math
# import argparse

_units = ["none", "deg C", "Pa", "%", "V", "", "mg", "mg", "mg", ""]
_types = [
    "none",
    "temperature",
    "pressure",
    "humidity",
    "battery",
    "etx",
    "acc_x",
    "acc_y",
    "acc_z",
    "move_count",
]
pdr_dict = {}

# Value processing


class SampleValue:
    def __init__(self, raw):
        self.type = raw[0]
        value_p = int.from_bytes(raw[1:5], "big", signed=True)
        value_q = int.from_bytes(raw[5:9], "big", signed=False)
        if value_q != 0:
            self.value = float(value_p) / value_q
        else:
            self.value = float(value_p)

    def __str__(self):
        if self.type < len(_types):
            return f"{_types[self.type]} = {self.value} {_units[self.type]}"
        else:
            return ""


class TagData:
    def __init__(self, raw):
        self.name = raw[0:16].decode("utf-8")
        self.parent = raw[16:56].rstrip(b"\x00").decode("utf-8")
        self.seq_no = int.from_bytes(raw[56:60], "big")
        self.sensors = []
        for sensor_start in range(60, len(raw), 9):
            self.sensors.append(SampleValue(raw[sensor_start : sensor_start + 9]))

    def __str__(self):
        sensor_str = " ".join([str(sensor) for sensor in self.sensors])
        return (
            f"{self.name}: parent = {self.parent} seq_no = {self.seq_no} {sensor_str}"
        )


# Packet receiver


def udp_server(host="::", port=7338):
    s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    s.bind((host, port))
    while True:
        (data, (host, port, flowinfo, scopeid)) = s.recvfrom(128 * 1024)
        yield (TagData(bytearray(data)), host, port)


def calculate_pdr(host, seq_no):
    global pdr_dict

    # Calculate the PDR
    if host in pdr_dict.keys():
        if seq_no == 0:
            pdr_dict[host] = 0
        else:
            pdr_dict[host] = pdr_dict[host] + 1
    else:
        if seq_no > 0:
            pdr_dict[host] = seq_no
        else:
            pdr_dict[host] = 0

    if seq_no == 0:
        pdr = 1.0
    else:
        pdr = float(pdr_dict[host]) / seq_no

    if pdr > 1:
        pdr_dict[host] = seq_no
        pdr = 1.0

    # print ("PDR: "+ pdr + "\n")
    return pdr * 100


# InfluxDB reporter


class DBReporter:
    def __init__(self, url, user, password, dbname, myorg):
        self.client = InfluxDBClient(url="http://localhost:8086", username=user, password=password, org=myorg)
        self.write_api = self.client.write_api(write_options=SYNCHRONOUS)

    def upload(self, tagdata, host, dbname, myorg):
        now = datetime.datetime.utcnow().isoformat() + "Z"
        body = []
        acc_avg = 0

        for sensor in tagdata.sensors:
            # print("DEBUG: TYPE:", sensor.type)
            if sensor.type < len(_types):
                if _types[sensor.type] == "etx":
                    fields = {
                        "etx": sensor.value,
                        "neigbour": tagdata.parent
                    }
                else:
                    fields = {
                        "value": sensor.value,
                    }
                if (
                    (_types[sensor.type] == "acc_x")
                    or (_types[sensor.type] == "acc_y")
                    or (_types[sensor.type] == "acc_z")
                ):
                    acc_avg += sensor.value * sensor.value
                body.append(
                    {
                        "measurement": _types[sensor.type],
                        "tags": {
                            "sensor": tagdata.name,
                            "address": host,
                        },
                        "time": now,
                        "fields": fields,
                    }
                )
            else:
                print("IndexError: skipping this value")

        # Add pdr
        pdr = calculate_pdr(host, tagdata.seq_no)
        body.append(
            {
                "measurement": "seq_no",
                "tags": {
                    "sensor": tagdata.name,
                    "address": host,
                },
                "time": now,
                "fields": {
                    "pdr": pdr,
                },
            }
        )

        # Add acc_rms
        acc_rms = math.sqrt(acc_avg)
        body.append(
            {
                "measurement": "acc_rms",
                "tags": {
                    "sensor": tagdata.name,
                    "address": host,
                },
                "time": now,
                "fields": {"value": acc_rms},
            }
        )
        self.write_api.write(dbname, myorg, body)
        now = datetime.datetime.now()

def parse_ipv6_address(string):
    try:
        expanded_ipv6 = ipaddress.IPv6Address(string).exploded
    except ValueError:
        msg = f"{string} is not a valid IPv6 address"
        raise ValueError(msg)
    return expanded_ipv6


def main():

    db = DBReporter("http://localhost:8086", "admin", "admin_password", "miradb", "myorg")

    for tagdata, host, port in udp_server():
        db.upload(tagdata, host, "miradb", "myorg")


if __name__ == "__main__":
    main()
