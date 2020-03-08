# ESP32

Language: English/[日本語](README-ja.md)

Let's play with [ESP32](https://www.espressif.com/en/products/hardware/esp32/overview).

## Preparation

I got an [ESP32 Feather](https://www.adafruit.com/product/3405), a development board from Adafruit (pre-soldered).

I am working on MacOS (10.14.6).

## ESP-IDF

GitHub repository: https://github.com/espressif/esp-idf

ESP-IDF is a set of tools to build an ESP32 program and write it in the flash memory.
It also contains FreeRTOS and APIs.

I submoduled ESP-IDF in this repository as [`esp-idf`](/esp-idf).

### Installing ESP-IDF

According to the [manual](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html), I took the following steps.

1. Move down to the `esp-idf` directory.

    ```
    cd esp-idf
    ```

2. Install ESP-IDF.

    ```
    ./install.sh
    ```

#### SSL error

Then I got an SSL error to download the following file.

```
https://github.com/espressif/openocd-esp32/releases/download/v0.10.0-esp32-20190313/openocd-esp32-macos-0.10.0-esp32-20190313.tar.gz
```

The warning message was

```
WARNING: Download failure [Errno socket error] [SSL: TLSV1_ALERT_PROTOCOL_VERSION] tlsv1 alert protocol version (_ssl.c:581)
```

This means Python is using TLS v1.0 that is rejected by a GitHub server.
The `install.sh` script runs Python2 by default and I have not been using and updated my Python2 recently.
So I wanted to use Python3 as described in [this issue](https://github.com/espressif/esp-idf/issues/4629).

I did not want to take an invasive approach; i.e., directly running `idf_tools.py`, so I decided to use a virtual environment.
I took the following steps,

1. Create a virtual environment with Python3.

    ```
    python3 -m venv ./venv
    ```

2. Activate the virtual environment.

    ```
    source ./venv/bin/activate
    ```

3. Run `install.sh`.

    ```
    ./install.sh
    ```

#### Another SSL error

Then I got a different error.

```
WARNING: Download failure <urlopen error [SSL: CERTIFICATE_VERIFY_FAILED] certificate verify failed: unable to get local issuer certificate (_ssl.c:1076)>
```

This is solved by installing certificate with the `Install Certificates` script bundled with a new Python distribution.
Previously I updated my Python3 with the [official distribution](https://www.python.org/downloads/release/python-376/).

#### virtualenv module error

Another problem arose.

It failed to install [`virtualenv`](https://virtualenv.pypa.io/en/latest/).

First I replaced `virtualenv` with `venv` in `idf_tools.py`, and it worked.
But I realized that [`virtualenv` has more features](https://virtualenv.pypa.io/en/latest/) than `venv`.
So I decided to respect the original implementation and install `virtualenv` prior to running `install.sh`.

```
pip install virtualenv
```

#### Python2

Wait, does it need Python2?
It works with Python3 so far.

## Downloading USB Driver

ESP32 Feather can be connected to a PC via USB.

ESP32 was not detected before I install an appropriate driver.
Running a commnad `ls /dev/cu.*` did not list ESP32.

One question was which USB driver I should install.
According to [this page](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/using-with-arduino-ide), I found that I needed [CP210X driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers) for ESP32 Feather.

After installing the driver, running a command `ls /dev/cu.*` listed `/dev/cu.SLAB_USBtoUART`.

## Installing Ninja

I installed [Ninja](https://ninja-build.org) via brew.

```
brew install ninja
```

As I succeeded to build a [hello-world example](https://github.com/espressif/esp-idf/tree/master/examples/get-started/hello_world) without Ninja, installation of Ninja may be optional.

## Writing a Project

[Example structure](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#example-project).

Before building a project, I had to run the following command,

```
source ./export.sh
```

## ESP-IDF Preparation Summary

Once you installed ESP-IDF, please take the following steps every time you work with ESP-IDF,

1. Move down to the `eps-idf` directory.

    ```
    cd eps-idf
    ```

2. Activate the virtual environment.

    ```
    source ./venv/bin/activate
    ```

3. Export ESP-IDF commands.

    ```
    source ./export.sh
    ```

## SPI

[This page](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html) describes SPI Mater Driver for ESP32.
[Here](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/spi_master) is also an example of an SPI master.

### ESP32 Feather pinouts

At first I had to understand the pinouts of ESP32 Feather.
According to the [schematics of ESP32 Feather](https://learn.adafruit.com/assets/41630) and [this section](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#gpio-matrix-and-io-mux) pinouts related to SPI are the following,

| Feather Label | GPIO# | SPI Function  |
|---------------|-------|---------------|
| SCK           | 5     | VSPICS0       |
| MO            | 18    | VSPICLK       |
| MI            | 19    | VSPIQ = MISO  |
| SDA           | 23    | VSPID = MOSI  |
| SCL           | 22    | VSPIWP (QSPI) |
| 21            | 21    | VSPIHD (QSPI) |

There are four SPI buses on ESP32 but we can use only two, HSPI (SPI2) and VSPI (SPI3), of them.
The above table shows the pinouts for the VSPI bus.

#### HSPI hook-up

According to the documentation, you may use the HSPI bus with the following pins,

| Feather Label     | GPIO# | SPI Function  |
|-------------------|-------|---------------|
| 15                | 15    | HSPICS0       |
| 14                | 14    | HSPICLK       |
| 12                | 12    | HSPIMISO      |
| 13 (goes to LED?) | 13    | HSPIMOSI      |
| N/A (GPIO2?)      | 2     | HSPIWP (QSPI) |
| A5                | 4     | HSPIHD (QSPI) |

I tested the hook-up shown above with ADXL345, but I strangely got a flash writer error if the GPIO12 pin was connected.

```
A fatal error occurred: Timed out waiting for packet content
CMake Error at run_cmd.cmake:14 (message):
  esptool.py failed
Call Stack (most recent call first):
  run_esptool.cmake:21 (include)
```

When I tried to monitor, I got the following message,

```
rst:0x10 (RTCWDT_RTC_RESET),boot:0x33 (SPI_FAST_FLASH_BOOT)
flash read err, 1000
ets_main.c 371
```

I found a related comment in [this page](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/) (seach GPIO12 in the page).
According to the comment [this page](https://github.com/espressif/esptool/wiki/ESP32-Boot-Mode-Selection) explains it.

My current understanding follows.

The GPIO12 pin is called a strapping pin (MTDI).
There are five strapping pins and their voltage affects the boot mode and other system settings.

The error message I got when I tried to monitor is described in [this section](https://github.com/espressif/esptool/wiki/ESP32-Boot-Mode-Selection#early-flash-read-error).

The `boot:0xXY` part of the message represents a combination of the following bits that explain which strapping pins are high.
- `0x01`: GPIO5
- `0x02`: MTDO (GPIO15)
- `0x04`: GPIO4 (not listed as a strapping pin)
- `0x08`: GPIO2
- `0x10`: GPIO0
- `0x20`: MTDI (GPIO12)

`boot:0x33` means `GPIO5 | MTDO | GPIO0 | MTDI`.

With VSPI hook-up I got `boot:0x13`; i.e., `GPIO5 | MTDO | GPIO0`.

The [solution would be complicated](https://esp32.com/viewtopic.php?t=5970), so I will go with VSPI so far.

#### GPIO mapping

It looks that GPIO numbers of ESP32 can be virtually mapped to any numbers with the [GPIO matrix](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#gpio-matrix-and-io-mux).
But According to the definition of IO_MUX, it is more efficient to use the default mappings without the GPIO matrix.

Here are some details I dug into.
- [This code](https://github.com/espressif/esp-idf/blob/c1d0daf36d0dca81c23c226001560edfa51c30ea/components/soc/soc/esp32s2/spi_periph.c) defines the default mappings of SPI.
- [This function](https://github.com/espressif/esp-idf/blob/4bfd0b961bea502a3bd2b0f64a933fbf87dc7349/components/driver/spi_common.c#L178-L192) checks whether specified GPIO numbers match the default.

### Key APIs

[spi_bus_initialize](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv418spi_bus_initialize17spi_host_device_tPK16spi_bus_config_ti)

This is the function that you call first.

[spi_bus_add_device](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv418spi_bus_add_device17spi_host_device_tPK29spi_device_interface_config_tP19spi_device_handle_t)

This function registers an SPI device.

[spi_device_polling_transmit](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv427spi_device_polling_transmit19spi_device_handle_tP17spi_transaction_t)

This is a handy function to transmit and receive data.

### SPI initialization

Here are common [`spi_bus_config_t`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv416spi_bus_config_t) settings for VSPI,
- `sclk_io_num`: `18`
- `mosi_io_num`: `23`
- `miso_io_num`: `19`
- `quadwp_io_num`: `-1` (disabled)
- `quadhd_io_num`: `-1` (disabled)

### ADXL345

[Here](./adxl345) is an example code for communication with an ADXL345.

### E-Paper Display

[Here](./epd) is an example code for communication with an e-paper display.