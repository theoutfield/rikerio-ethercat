# EtherCAT Master for RikerIO

This software is a implementation for the RikerIO Linux Automation Framework in order to use EtherCAT Networks. RikerIO-EtherCAT uses the Simple Open EtherCAT Master from the Open EtherCAT Society.

## Build and Install

``` 
mkdir build & cd build
cmake ..
make
make install
```

## Usage

### Freerun

To just run the EtherCAT Master and observe IO Data simply execute the following command.

`ethercat run --ifname=eth0`

The PDOs from the EtherCAT network will be mapped in to a shared memory area located in /dev/shm/io.mem. In another terminal, monitor the IOs with hexdump for example `hexdump -C /dev/shm/io.mem`.

### Network Scan

You can scan the EtherCAT Network and receive a list of slaves. Save the output to a .yaml file to use it as a configuration for grouping slaves and linking PDOs.

`ethercat scan --ifname=eth0`

### RikerIO Master

To start the EtherCAT Master as a RikerIO Master type `ethercat rikerio --ifname=eth0`. The default id is ethercat. You can change the id with the --id option. You can also use a configuration from a network scan with the --config option. To set a memory offset use the --offset option.
