#! /usr/bin/env bash

internet_interface=eth0
wireless_interface=wlan1


if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

echo 1 > /proc/sys/net/ipv4/ip_forward
iptables -F
iptables -t nat -F
iptables -t mangle -F
iptables -t nat -A POSTROUTING -o $internet_interface -j MASQUERADE
iptables -A FORWARD -i $wireless_interface -j ACCEPT
 
