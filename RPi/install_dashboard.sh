#!/bin/bash

if [ "$(id -u)" -ne 0 ]; then
        echo 'This script must be run by root' >&2
        exit 1
fi

arch=arm64
config=/etc/mira-monitor/config.toml
docker compose up -d --build
sleep 8s
token_json=$(docker exec influxdb2 influx auth create --skip-verify --org myorg --read-buckets --write-buckets --json)
token=$(echo "$token_json" | grep -oP '(?<="token": ")[^"]+')

docker exec influxdb2 influx bucket create --skip-verify --name miradb --org myorg

dpkg -i mira-gateway-${arch}.deb
dpkg -i mira-monitor-${arch}.deb

systemctl stop mira-monitor-receiver.service
systemctl stop mira-monitor-api.service
systemctl stop mira-gateway.service

sed -i "s/^auth_token = .*/auth_token = \"${token}\"/" "$config"
sed -i "s/token: .*/token: $token/" ./services/grafana/datasource.yml

docker restart grafana
cp mira-gateway /etc/default/mira-gateway
cp mira-gateway.env /etc/mira-gateway/mira-gateway.env

systemctl start mira-gateway.service
systemctl start mira-monitor-receiver.service
systemctl start mira-monitor-api.service