# ESP32

Let's play with [ESP32](https://www.espressif.com/en/products/hardware/esp32/overview).

I got a [development board from Adafruit (pre-soldered)](https://www.adafruit.com/product/3405).

I am working on MacOS (10.14.6).

## Installing ESP-IDF

ESP-IDF: https://github.com/espressif/esp-idf

According to the manual, I ran the following command.

```
./install.sh
```

**Then I got an SSL error to download the following file.**

```
https://github.com/espressif/openocd-esp32/releases/download/v0.10.0-esp32-20190313/openocd-esp32-macos-0.10.0-esp32-20190313.tar.gz
```

```
WARNING: Download failure [Errno socket error] [SSL: TLSV1_ALERT_PROTOCOL_VERSION] tlsv1 alert protocol version (_ssl.c:581)
```

This means Python is using TLS v1.0 that is rejected by a GitHub server.
The `install.sh` runs Python2 by default and I have not updated my Python2 recently.
So I wanted to use Python3 as described in [this issue](https://github.com/espressif/esp-idf/issues/4629).

I did not want to directly run `idf_tools.py`, so I took the following steps,

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

**Then I got a different error.**

```
WARNING: Download failure <urlopen error [SSL: CERTIFICATE_VERIFY_FAILED] certificate verify failed: unable to get local issuer certificate (_ssl.c:1076)>
```

This is solved by installing certificate with the `Install Certificates` script bundled with a new Python distribution.

**Another problem arose.**

It failed to install [`virtualenv`](https://virtualenv.pypa.io/en/latest/).

First I replaced `virtualenv` with `venv` in `idf_tools.py`, and it worked.
But I realized that `virtualenv` has more feature than `venv`.
So I decided to respect the original implementation and install `virtualenv` prior to running `install.sh`.

```
pip install virtualenv
```

## Downloading USB driver

ESP32 was not detected before I install an appropriate driver.
Running a commnad `ls /dev/cu.*` did not list ESP32.

According to [this page](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/using-with-arduino-ide), I though I needed [CP210X driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers).

After installing the driver, running a command `ls /dev/cu.*` listed `/dev/cu.SLAB_USBtoUART`.

## Ninja

I installed [Ninja](https://ninja-build.org) via brew.

```
brew install ninja
```

## Python2

Wait, does it need Python2?
It works with Python3 so far.