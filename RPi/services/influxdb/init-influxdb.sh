#!/bin/bash
set -e

influx -execute "CREATE USER mirauser WITH PASSWORD 'mypassword'"
influx -execute "CREATE DATABASE miradb"
influx -execute "GRANT ALL ON miradb TO mirauser"
