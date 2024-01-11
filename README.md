![CI](https://github.com/blueknobby/bkt1-firmware/workflows/CI/badge.svg)


# BKT Throttle

This is the firmware source code for the BKT-1 wireless throttle from Blue
Knobby Systems.


## License & Non-Disclosure

Most of this software is licensed under the GNU General Public License,
version 2.0.  Some components are released under other licenses.  A few
small scripts and supporting files are in the public domain.


## Development System Requirements

This application is based on the Espressif ESP-IDF framework, and that must
be installed on your development system.  The BKT code currently requires
ESP-IDF v5.1.2 (or possibly later).  The [Getting
Started](https://docs.espressif.com/projects/esp-idf/en/v5.1.2/get-started/index.html)
pages from Espressif cover the requirements for the IDF and procedures to
get the software installed.

The BKT code uses the CMake build system exclusively.

99% of the development is done on a Mac, but I regularly test build the
code on a Raspberry Pi.  I find occasional case-sensitivity issues because
to the Mac ```<ArduinoJson.h>``` and ```<ArduinoJSON.h>``` are the same
header file, but not on Linux.  In theory, this can be built under Windows,
but I've never done that.

The throttle depends on an external app for configuration & setup.  Right
now, the only app available runs on iOS (and I guess theoretically a Mac
too now, via Catalyst).  So you'll probably need a Mac just to get the app
compiled and running on your phone or tablet.


## Getting the source code

All source is maintained on Github at
https://github.com/blueknobby/bkt1-firmware.git.  This repository contains
references to other repositories, so you need to check this out using the
`--recursive` option.

```sh
$ git clone --recursive https://github.com/blueknobby/bkt1-firmware.git
```

## Building the source code

Once you have the IDF installed and set up in your current shell (via
export.sh), building this code should be simple.

```sh
$ cd blue-knobby-throttle
$ ./all-from-scratch
```

This is a good test of any changes you made, as this command will clean out
the build directory, rebuild the config file and then perform a clean
build.  If you have ```$ESPPORT``` set in your environment, it will also
flash the code to your device and run it.

While running the code, I find that most of the time I just rebuild from
within the IDF monitor application (```C-t C-a```).  It usually takes
longer to reflash than it does to perform the build & link cycle.

Otherwise you can build using the ```idf.py``` application.  This command
gets used a lot:

```sh
$ idf.py build flash monitor
```

and in fact that is the core of ./all-from-scratch.


## Hardware Compatilibility

This code is developed for and has been tested on Revision A of the BKT-1
hardware.  You can run this on other ESP32-S3 hardware devices, and the
[Espressif ESP32-S3
DevKitC](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html)
has been used to test many small bits of the code.  The specific hardware
types are identified in fuse bits set upon initial bringup of a new board.

The intention is that this code base should support all variants of the
throttle that are based on an Espressif chip that uses the IDF.

Non-generic code (such as pin assignments) should be conditional on the
board type as determined at runtime.  WiFi/BLE interface development &
testing can certainly proceed using a board like the DevKit-C.


## How to Contribute

If you haven't used git before, then that's a hurdle to leap just to make
some progress.  I like [Scott Chacon's book Pro Git](https://git-scm.com/book/en/v2) as being fairly readable and a good
overview of what git does.

As the source is hosted on Github, using [Github's native
flow](https://guides.github.com/introduction/flow/) ought to work well.
Please [fork the main repository](https://guides.github.com/activities/forking/) to your own
account, and then do your work in a branch in your own repository.  When
you're ready to submit code to inclusion into the main repository, submit a
pull request.  Once the maintainer approves of the request, he will merge
it into the main repository onto the ```main``` branch.

The ```main``` branch is *always* potentially releaseable -- it should
*always* work.

You will then have to pull from the main repository into your fork, make
another branch for further work, and repeat the process.  It's a pain until
you get used to it.  The most important rule to remember is this:

> *DO NOT WORK IN THE main BRANCH*

Always do your work in a feature branch and merge that into ```main``` when the
code is ready.


## Communications

Email often works, but I am known to drown in it from time to time.

Try me at `david zuhn <zoo@blueknobby.com>`.
