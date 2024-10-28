#!/bin/bash

if [ "$(id -u)" -ne 0 ]; then
        echo 'This script must be run by root' >&2
        exit 1
fi
if [ "$(uname -m)" != "aarch64" ]; then
        echo 'This script must be run on 64 bit operating systems' >&2
        exit 1
fi

config=/etc/mira-monitor/config.toml
gen_pan_id=../src/vendor/libmira/tools/mira_gen_panid.py
gw_credentials_file=/etc/mira-gateway/mira-gateway.env

mkfile() { mkdir -p "$(dirname "$1")" && touch "$1" ;  }

docker compose up -d --build
sleep 8s
token_json=$(docker exec influxdb2 influx auth create --skip-verify --org myorg --read-buckets --write-buckets --json)
token=$(echo "$token_json" | grep -oP '(?<="token": ")[^"]+')

docker exec influxdb2 influx bucket create --skip-verify --name miradb --org myorg

dpkg -i mira-gateway-arm64.deb
dpkg -i mira-monitor-arm64.deb

systemctl stop mira-monitor-receiver.service
systemctl stop mira-monitor-api.service
systemctl stop mira-gateway.service

sed -i "s/^auth_token = .*/auth_token = \"${token}\"/" "$config"
sed -i "s/token: .*/token: $token/" ./services/grafana/datasource.yml

docker restart grafana
cp mira-gateway /etc/default/mira-gateway

if [ -f "$gen_pan_id" ]; then
        pan_id=$(python $gen_pan_id | cut -c 3-)
        net_key=$(hexdump -n 16 -e '"%02x"' /dev/urandom)
        mkfile $gw_credentials_file
        printf "MIRA_GW_PAN_ID="$pan_id"\nMIRA_GW_KEY="$net_key"\n" > $gw_credentials_file
        echo "Writing generated network credentials to:" $gw_credentials_file
        cat $gw_credentials_file
else
        echo "Could not find mira_gen_panid.py"
        echo "Writing default network credentials to:" $gw_credentials_file
        mkfile $gw_credentials_file
        cp mira-gateway.env $gw_credentials_file
        cat $gw_credentials_file
fi

systemctl start mira-gateway.service
systemctl start mira-monitor-receiver.service
systemctl start mira-monitor-api.service