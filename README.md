kcshdproxy device
=================
This package contains a proxy device driver for AmigaOS that makes it possible
to read from and write to emulated MS-DOS hard drive partitions that are managed
by the [KCS Power PC board](https://amiga.resource.cx/exp/powerpc) emulator.

Background
==========
The Amiga platform and operating system are highly flexible and extensible.
For example, file system support is modular and extensible -- AmigaOS supports
multiple filesystems through filesystem handlers. Custom filesystem handlers
(such as CrossDOS and fat95) make it possible to manage MS-DOS floppy disks
from AmigaOS.

Moreover, an Amiga 500 can be extended with a KCS PowerPC board to emulate an
XT PC. In addition, this PC emulator integrates with many kinds of Amiga
peripherals, such as its keyboard, mouse and hard disk (provided through an
expansion board).

Unfortunately, it is not possible to use any of the existing FAT file system
handlers to get access to the emulated PC partitions through AmigaOS. This is
quite inconvenient if you want to exchange files between your Amiga and PC hard
drive partitions.

There are two reasons why it is not possible to get access to an emulate PC
hard drive partition by conventional means:

* The contents of a PC hard-drive is stored on a native Amiga partition and
  contains a MBR partition table. Some filesystem handlers do not know how to
  interpret a partition table
* The order of each pair of bytes is reversed -- this choice probably has
  something to with performance -- the Motorola 68000 is a big endian CPU and an
  Intel 8086 is a little endian CPU. The emulator software (running on the 68000
  CPU) needs to process numbers from a PC hard-drive and is most likely
  performing faster if it does not need to reverse bytes at runtime.

This proxy device driver makes it possible to cope with the above two
limitations, by offering the following features:

* Unreversing each pair of bytes. The proxy device driver intercepts relevant
  SCSI and trackdisk I/O requests and reverses their data payloads so that
  the byte order is in the original format. File system handlers should then
  be able to understand the data.
* Offset translation. It can translate offset values in such a way that the
  beginning of the emulated PC hard drive starts at 0. Using this property
  makes it possible to use the contents of an KCS Amiga partition as a virtual
  PC hard disk device.
* A helper program (`searchmbr`) that can tell you the position of the MBR
  inside an Amiga partition
* A helper program (`querypcparts`) that can tell you the offset and sizes of
  each MS-DOS partition so that it can be easily configured.

This proxy device driver should work with any kind of FAT file system DOS driver
for AmigaOS. It was tested with CrossDOS (included with Workbench 2.0 and newer)
and [fat95](https://aminet.net/package/disk/misc/fat95)

Disclaimer
==========
Although I took great care in testing all kinds of scenarios, I have only tested
the tools with two kinds of SCSI drivers: `evolution.device` (which is used by the
[MacroSystem Evolution](https://www.edsa.uk/blog/macro-systems-evolution-harddisk-controller)
expansion board) and FS-UAE's `uaehf.device`.

There many kinds of SCSI controllers supported by the KCS PowerPC board, but I
do not have the means to test them. I do not believe there is any reason why
they should not work, but use this package at your own risk!

Installation
============
Installation of this package is straight forward. Download the archive from the
GitHub releases page. Unpack the archive and copy `kcshdproxy.device` to `DEVS:`.

Building from source code
=========================
If you need to build the package from source code, then you need the SAS C
compiler for AmigaOS (I have used version 6.58).

Change your current working folder to the checkout and run the following
command-line instruction to build the binaries:

```
> smake
```

Configuring a virtual PC hard drive
===================================
To configure a virtual PC hard drive, you first need to determine a number of
relevant configuration properties of your KCS Amiga partition.
[SysInfo](https://sysinfo.d0.se) is a program that can easily help you to
determine them.

We can get an overview of drives, by starting SysInfo and clicking on the
'DRIVES' button. In this screen, you should select your KCS Amiga partition.
You should memorize the following properties:

* Unit number:       `0`
* Device name:       `evolution.device`
* Surfaces:          `1`
* Sectors per side:  `544`
* Reserved blocks:   `2`
* Lowest cylinder:   `376`
* Number of buffers: `130`

In the above overview, example values are given that correspond to my
configuration.

You typically also need to known the block size. This value always seems to the
same:

```
Block size = 512
```

From the above properties, you need to compute the offset (in bytes) of the
virtual PC drive with following formula:

```
Offset = Surfaces * Sectors per side * Lowest cylinder * Block size
```

In my example, applying the formula yields:

```
Offset = 1 * 544 * 376 * 512 = 104726528
```

After determining the partition offset, we must determine the offset of the MBR
block. Typically, it is at the beginning of the partition but there may be
situations (e.g. when using Kickstart 1.3) that the MBR is moved somewhat. The
`searchmbr` tool can help you to determine the exact offset:

```
> searchmbr evolution.device 0 104726528
Checking block at offset: 104726528
MBR found at offset: 104726528
```

In the above example, we invoke the `searchmbr` tool with the following
parameters:

* The device to use is named: `evolution.device`
* We want to access unit: `0`
* We want to start searching from the partition's start offset: `104726528`

In the above example, we get a confirmation that the MBR starts at the beginning
of the partition

Finally, we must create a configuration file `S:KCDHDProxy-Config` that instructs
the HD proxy driver to get access to the KCS partition. This configuration file
consists of two lines (that should end with a linefeed):

* The first line refers to the *offset* of the KCS Amiga partition in bytes (the
  output of the `searchmbr` tool)
* The second line refers to the hard disk's *device driver*. This value should
  correspond to the `Device name` field in SysInfo. In my case it is called:
  `evolution.device`. If the second-line is omitted, then the driver uses
  its internal default value: `scsi.device`.

For my configuration, I need to create the following configuration file:

```
104726528
evolution.device
```

Configuring mount entries
=========================
After configuring the virtual hard disk device, we need to set up mount entries
for each PC partition so that AmigaDOS can access it.

To do this, you need to consult the documentation of your MS-DOS file system
handler. Then you must augment and override certain configuration properties.

In this section, two example scenarios are given: one using CrossDOS and the
other using fat95.

### Example: creating a basic CrossDOS mount entry

CrossDOS only works with Workbench 2.0 or newer.

Do the following steps to create a mount entry for device `PCDH0:`:
* Copy `DEVS:DOSDrivers/PC0` to `DEVS:DOSDrivers/PCDH0`
* Copy `DEVS:DOSDrivers/PC0.info` to `DEVS:DOSDrivers/PCDH0.info`
* Request the icon information of PCDH0.info (`Icons -> Information`). Remove
  `UNIT=0` tooltip.
* Open `DEVS:DOSDrivers/PCDH0` in a text editor to make modifications

### Example: creating a basic fat95 mount entry

Do the following steps to create a mount entry for device `PCDH0:`:
* On Workbench 1.3, add a `PCDH0:` entry to `DEVS:MountList` and copy the
  contents of `fat95/english/MS0` with a text editor
* On Workbench 2.0 or newer, copy `fat95/english/MS0` and
  `fat95/english/MS0.info` to `DEVS:DOSDrivers/PCDH0` and
  `DEVS:DOSDrivers/PCDH0.info`
* Open the configuration in a text editor

To work with a hard disk partition, we must change the following property:

```
DosType = 0x46415401
```

To auto-detect a partition, we must change the following properties to:

```
LowCyl  = 0
HighCyl = 1
```

### Configuring common values

There are some values that need to be changed to common values. Change or add
them to correspond to the following values:

```
Device         = kcshdproxy.device
BlocksPerTrack = 1
Surfaces       = 1
BlockSize      = 512 /* Is always the same */
```

As explained earlier, `BlockSize` should always be `512` and we need our proxy
driver: `kcshdproxy.device` as a `Device` to reverse each byte pair and translate
offsets.

For `BlocksPerTrack` and `Surfaces` we use `1`, because the start offset of a PC
partition is typically not aligned to a cylinder boundary of an Amiga partition.
Instead, we must address the offsets in sectors.

### Configuring SysInfo properties

We must override/augment some configuration properties of the mount configuration
with properties retrieved from SysInfo:

```
Unit     = 0   /* Should correspond to: Unit number */
Reserved = 2   /* Should correspond to: Reserved blocks */
Buffers  = 130 /* Should correspond to: Number of buffers */
```

### Configuring HDToolBox properties

Some drivers require the `Mask` and `MaxTransfer` values. We can retrieve these
properties by opening `HDToolBox` (in Workbench 2.0 or newer this tool can be
found in `SYS:Tools`), selecting the KCS partition and then the option:
`Advanced options -> Change File System for Partition`.

In my setup these values correspond to:
* `Mask`:        `0xffffff`
* `MaxTransfer`: `0xfffffe`

We can add/overrides these properties to the mount configuration as follows:

```
Mask        = 0xffffff
MaxTransfer = 0xfffffe
```

### Configuring the offsets of a PC partition

fat95 supports the auto detection of the location of a PC partition, but
CrossDOS does not. In such cases, we need to manually specify the partition
offsets. We can use the `querypcparts` program (included with this package) to
assist with that:

```
> querypcparts 0
Partition: 1, first sector offset: 34, number of sectors: 202878
```

The command-line parameter: `0` refers to the device unit number. If this
parameter is omitted, it defaults to `0`.

In my example case, there is only one MS-DOS partition. It resides at offset
`34`. Its size is `202878` sectors.

We can compute the high cylinder with the following formula:

```
HighCyl = First sector offset + Number of sectors - 1
```

With the above information, we can configure cylinder properties in the mount
configuration as follows:

```
LowCyl  = 34     /* Must correspond to the first sector offset */
HighCyl = 202911 /* Result of the formula */
```

### Other file system handlers

If you want to use a different filesystem handler, then consult the
documentation of your file system handler. Once the configuration settings have
been applied, augment and override the configuration properties as shown in the
earlier sections.

License
=======
The deployment recipes and configuration files in this repository are
[MIT licensed](./LICENSE.txt).
