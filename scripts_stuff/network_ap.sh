#! /usr/bin/env bash

ip_address="10.0.0.1"

if [ $EUID -ne 0 ]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

ifconfig wlan1 up $ip_address

if [ $? -ne 0 ]; then 
    echo "An error occured."
    exit
fi

service hostapd restart
service udhcpd restart

#then nat stuff


