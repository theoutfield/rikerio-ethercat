# EtherCAT Master for RikerIO

This software is a implementation for the RikerIO Linux Automation Framework in order to use EtherCAT Industrial Communication Protocl. RikerIO-EtherCAT uses the Simple Open EtherCAT Master from the Open EtherCAT Society [SOEM](https://github.com/OpenEtherCATsociety/SOEM).

## Build and Install

If you installed RikerIO onto your system, use the following cmake flags.

``` 
mkdir build & cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make & make install
```

In case you need the RikerIO Library build within the EtherCAT Master as a static library use the `WITH_RIO=ON` flag. And in case you want to build for ARM, there is a CMake toolchain file in the build-tools folder. Call CMake with `-DCMAKE_TOOLCHAIN_FILE=path/to/build-tools/Toolchain-arm.cmake`. You need to install the approprate compiler tools of course.

## Usage

### Freerun

To just run the EtherCAT Master and observe IO Data simply execute the following command.

`ethercat run --ifname=eth0`

The PDOs from the EtherCAT network will be mapped in to a shared memory area located in /dev/shm/io.mem. In another terminal, monitor the IOs with hexdump for example `hexdump -C /dev/shm/io.mem`.

### Network Scan

You can scan the EtherCAT Network and receive a list of slaves. Save the output to a .yaml file to use it as a configuration for grouping slaves and linking PDOs.

`ethercat scan --ifname=eth0`

### Apply Slave Configurations

You can specify Slave Configurations with the `sconf=config.yaml` option. Have a look at the etc/slave-config.yaml file.

### RikerIO Master

To start the EtherCAT Master as a RikerIO Master type `ethercat rikerio --ifname=eth0`. The default id is ethercat. You can change the id with the --id option. You can also use a configuration from a network scan with the --config option.

### RikerIO Links

When storing the configuration file as a .yaml file, you can add multiple RikerIO Links to each Slave PDO.

## Example

A normal setup of an EtherCAT Network for a RikerIO Application looks like this.

First you scan your network and apply a slave configuration file:

````
ethercat scan --ifname=eth0 --sconf=slave-config.yaml > ec-config.yaml
````

Second add your RikerIO Link names to the appropriate PDOs in the ec-config.yaml and third, add the EtherCAT Master as a Master to your RikerIO Process Configuration under /etc/rikerio/config.yaml like so:

````
processes:
- timeout: 2
  master:
  - name: "ec-master-01"
    cmd: "/usr/bin/ethercat"
    args:
      - "rikerio"
      - "--id=ec-master-01"
      - "--ifname=eth0"
      - "--config=path/to/ec-config.yaml"
      - "--sconf=path/to/slaves-config.yaml"
    priority: 1

````

# Disclaimer

RikerIO-EtherCAT is provided by Cloud Automation "as is" and "with all faults." Cloud Automation makes no representations or warranties of any kind concerning the safety, suitability, lack of viruses, inaccuracies, typographical errors, or other harmful components of RikerIO-EtherCAT. There are inherent dangers in the use of any software, and you are solely responsible for determining whether RikerIO-EtherCAT is compatible with your equipment and other software installed on your equipment. You are also solely responsible for the protection of your equipment and backup of your data, and Cloud Automation will not be liable for any damages you may suffer in connection with using, modifying, or distributing RikerIO-EtherCAT.

# Legal Notice

The EtherCAT Technology is covered, including but not limited to the following patent applications and
patents: EP1590927, EP1789857, DE102004044764, DE102007017835 with corresponding applications or
registrations in various other countries.
The TwinCAT Technology is covered, including but not limited to the following patent applications and
patents: EP0851348, US6167425 with corresponding applications or registrations in various other countries.

Beckhoff and EtherCAT are registered trademarks of and licensed by Beckhoff Automation GmbH.
