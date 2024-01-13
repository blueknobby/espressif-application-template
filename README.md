![CI](https://github.com/blueknobby/espressif-application-template/workflows/CI/badge.svg)


# Application Template

This is a starting point for applications on Espressif ESP32 family
microcontrollers.


## License & Non-Disclosure

Most of this software is licensed under the GNU General Public License,
version 2.0.  Some components are released under other licenses.  A few
small scripts and supporting files are in the public domain.  See the
header of each file for details.


## Development System Requirements

This application is based on the Espressif ESP-IDF framework, and that must
be installed on your development system.  This code is tested against
ESP-IDF v5.1.2 (or possibly later).

The [Getting Started](https://docs.espressif.com/projects/esp-idf/en/v5.1.2/get-started/index.html)
pages from Espressif cover the requirements for the IDF and procedures to
get the software installed.

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
https://github.com/blueknobby/espressif-application-template.git.

```sh
$ git clone https://github.com/blueknobby/espressif-application-template.git
```

## Building the source code

Once you have the IDF installed and set up in your current shell (via
export.sh), building this code should be simple.

```sh
$ cd espressif-application-template
$ ./all-from-scratch
```

This will build the application for EACH of the supported platforms in the
esp-idf package.  You can specify a subset of the platforms to build as
arguments to `all-from-scratch`.

```sh
$ ./all-from-scratch esp32s3
```

This is a good test of any changes you made, as this command will clean out
the build directory, rebuild the config file and then perform a clean
build.

Once fully built, rebuilds or flashing the board can readily be done
with the `idf.py` command using the `-B` flag:

```sh
$ idf.py -B esp32s3 flash monitor
```

While running the code, I find that most of the time I just rebuild from
within the IDF monitor application (```C-t C-a```).  It usually takes far
longer to reflash than it does to perform the build & link cycle.

* [ESP32-S3-DevKitC](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html)
* [ESP32-C3-DevKitC](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/hw-reference/esp32c3/user-guide-devkitc-02.html)
* [ESP32-H2-DevKitM](https://docs.espressif.com/projects/espressif-esp-dev-kits/en/latest/esp32h2/esp32-h2-devkitm-1/index.html)
* [Adafruit Huzzah32 Feather](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather)

has been used to test many small bits of the code.

The intention is that this code base should be generic enough to support
all variants of ESP32 boards, with appropriate modifications to the
`sdkconfig.defaults` files to describe the hardware.

Non-generic code (such as pin assignments) can also be conditional on the
board type as determined at runtime.  WiFi/BLE interface development &
testing can certainly proceed using a board like the DevKit-C.


## How to Contribute

If you haven't used git before, then that's a hurdle to leap just to make
some progress.  I like [Scott Chacon's book Pro
Git](https://git-scm.com/book/en/v2) as being fairly readable and a good
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

Email usually works, but I am known to drown in it from time to time.

Try me at david zuhn <zoo@blueknobby.com>
