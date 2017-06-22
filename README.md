# EtherCAT Master for RikerIO

This software is a implementation for the RikerIO Linux Automation Framework in order to use EtherCAT Networks. RikerIO-EtherCAT uses the Simple Open EtherCAT Master from the Open EtherCAT Society.

## Usage

ethercat freerun --ifname=<network interface>

In another terminal, monitor the IOs with hexdump for example `hexdump -C /dev/shm/io.mem'.


