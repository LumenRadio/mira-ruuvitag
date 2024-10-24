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
# Calculate number of hops towards root.


class CalculateNbrOfHops:
    def __init__(self, root):
        self.dict_address_parent = {}
        self.root_address = root

    def calc_hops(self, host, parent_address):
        # key is the host, value is the parent
        self.dict_address_parent[host] = parent_address
        # Will add if the host if not present, otherwise update

        # If we have a root address given, use that
        if self.root_address:
            nbr_of_hops = 1
            # Expand ipv6 addresses from possibly abbreviated form before comparing
            while parse_ipv6_address(parent_address) != parse_ipv6_address(
                self.root_address
            ):
                nbr_of_hops = nbr_of_hops + 1

                parent_address = self.dict_address_parent.get(parent_address)
                if parent_address == None or nbr_of_hops > 30:
                    # Safety if stuck in loop or parent is not present
                    return -1
            return nbr_of_hops
        # If no root address given, just follow the parents up until we hit None. Most likely this is root
        else:
            nbr_of_hops = 0
            while parent_address != None:
                nbr_of_hops += 1
                parent_address = self.dict_address_parent.get(parent_address)
                if nbr_of_hops > 30:
                    # Safe if stuck in loop
                    return -1

            return nbr_of_hops


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
    def __init__(self, url, user, password, dbname, myorg, root):
        self.client = InfluxDBClient(url="http://localhost:8086", username=user, password=password, org=myorg)
        self.write_api = self.client.write_api(write_options=SYNCHRONOUS)
        self.calc_hops = CalculateNbrOfHops(root)

    def upload(self, tagdata, host, dbname, myorg):
        hops = self.calc_hops.calc_hops(host, tagdata.parent)
        now = datetime.datetime.utcnow().isoformat() + "Z"
        body = []
        acc_avg = 0

        for sensor in tagdata.sensors:
            # print("DEBUG: TYPE:", sensor.type)
            if sensor.type < len(_types):
                if _types[sensor.type] == "etx":
                    fields = {
                        "etx": sensor.value,
                        "neigbour": tagdata.parent,
                        "hops": hops,
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
        print(
            "[{}] host: {}, hops: {}, sensor: {}".format(
                now.strftime("%H:%M:%S"), host, hops, tagdata
            )
        )


def parse_ipv6_address(string):
    try:
        expanded_ipv6 = ipaddress.IPv6Address(string).exploded
    except ValueError:
        msg = f"{string} is not a valid IPv6 address"
        raise ValueError(msg)
    return expanded_ipv6


def main():
    # parser = argparse.ArgumentParser(description="UDP to influx2)
    # token = ""
    # parser.add_argument(
    #     "-t",
    #     "--token",
    #     dest=token,
    #     help="Influxdb2 auth token",
    #     required=True)
    
    root_addr = os.environ.get("ROOT_ADDR", None)
    if root_addr is not None and root_addr != "":
        root_addr = parse_ipv6_address(root_addr)
        print(f"Starting udp-to-influx with root address set to {root_addr}")
    else:
        print(
            f"Starting udp-to-influx with no root address set. Falling back on automatic detection of root for hops calculations."
        )

    db = DBReporter("http://localhost:8086", "admin", "admin_password", "miradb", "myorg", root_addr)

    for tagdata, host, port in udp_server():
        db.upload(tagdata, host, "miradb", "myorg")


if __name__ == "__main__":
    main()
