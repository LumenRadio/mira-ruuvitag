import socket
#import influxdb
import datetime

# Value processing

class SampleValue:
    units = ['none', 'deg C', 'Pa', '%', 'V', '', '']
    types = ['none', 'Temperature', 'Pressure', 'Humidity', 'Battery', 'ETX', 'Clock drift']

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
        #self.host = host
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
    types = ['none', 'Temperature', 'Pressure', 'Humidity', 'Battery', 'ETX', 'Clock drift']

    def __init__(self, host, port, user, password, dbname):
        self.client = influxdb.InfluxDBClient(host, port, user, password, dbname)


    def upload(self, tagdata):
        now = datetime.datetime.utcnow().isoformat() + 'Z'
        body = []
        for sensor in tagdata.sensors:

            if sensor.types[sensor.type] == 'ETX':
                fields = {
                    "etx": sensor.value,
                    "neigbour": tagdata.parent
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
        #print body
        self.client.write_points(body)


# Main

db = DBReporter('localhost', 8086, 'monitoring', 'uploadstuff', 'monitoring')

for tagdata, host, port in udp_server():
    now = datetime.datetime.now()
    print "[", now.strftime("%H:%M:%S"), "] host:", host, "sensor:", tagdata
    #db.upload(tagdata)
    db.upload2(tagdata)
