#!/bin/sh

ifconfig eth0 0.0.0.0 down
ifconfig eth1 0.0.0.0 down
#ifconfig eth2 0.0.0.0 down
#ifconfig eth3 0.0.0.0 down

#ifconfig bond0 10.4.53.196 netmask 255.255.255.0 up
ifconfig bond0 192.168.0.5 netmask 255.255.255.0 up
ifenslave bond0 eth0 eth1 
#eth2 eth3 
#eth2 eth3

ifconfig bond0 down
echo balance-alb > /sys/class/net/bond0/bonding/mode
ifconfig bond0 up
#ifconfig bond0 10.4.53.196 netmask 255.255.255.0 up

#echo +eth1 > /sys/class/net/bond0/bonding/slaves
#echo +eth2 > /sys/class/net/bond0/bonding/slaves
#echo +eth3 > /sys/class/net/bond0/bonding/slaves

#echo 2 > proc/irq/smp_affinity

