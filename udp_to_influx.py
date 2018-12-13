import socket
import influxdb
import datetime

# Value processing

class SampleValue:
    units = ['none', 'deg C', 'Pa', '%', 'V']

    def __init__(self, raw):
        self.name = str(raw[0:15]).strip('\0')
        self.unit = raw[15]
        value_p = sum([v<<(8*(3-i)) for i,v in enumerate(raw[16:20])])
        if value_p & 0x80000000:
            value_p -= 0x100000000
        value_q = sum([v<<(8*(3-i)) for i,v in enumerate(raw[20:24])])
        self.value = float(value_p) / value_q

    def __str__(self):
        return "%s = %f %s" % (self.name, self.value, self.units[self.unit])

class TagData:
    def __init__(self, raw):
        self.name = str(raw[0:32]).strip('\0')
        self.sensors = []
        for sensor_start in range(32,len(raw),24):
            self.sensors.append(SampleValue(raw[sensor_start:sensor_start+24]))

    def __str__(self):
        return self.name + ": " + " ".join(str(sensor) for sensor in self.sensors)

# Packet receiver

def udp_server(host='::', port=7337):
    s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    s.bind((host, port))
    while True:
        (data, (host, port, flowinfo, scopeid)) = s.recvfrom(128*1024)
        yield (TagData(bytearray(data)), host, port)

# InfluxDB reporter

class DBReporter:
    def __init__(self, host, port, user, password, dbname):
        self.client = influxdb.InfluxDBClient(host, port, user, password, dbname)

    def upload(self, tagdata):
        now = datetime.datetime.utcnow().isoformat() + 'Z'
        body = []
        for sensor in tagdata.sensors:
            body.append({
                "measurement": sensor.name,
                "tags": {
                    "sensor": tagdata.name,
                },
                "time": now,
                "fields": {
                    "value": sensor.value
                }
            })
        self.client.write_points(body)

# Main

db = DBReporter('localhost', 8086, 'monitoring', 'uploadstuff', 'monitoring')

for tagdata, host, port in udp_server():
    print tagdata
    db.upload(tagdata)
