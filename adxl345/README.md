# ADXL345

Language: English/[日本語](README-ja.md)

ADXL345 is a 3-axis accelerometer.

The [datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf).

## Development Board

I got [this](http://akizukidenshi.com/catalog/g/gM-06724/).

## Hook-up

Here is my hook-up between ESP32 Feather and ADXL345.

| Feather Label | ADXL345 |
|---------------|---------|
| 3V            | VDD     |
| GND           | GND     |
| SCK           | CS      |
| MO            | SCL     |
| MI            | SDO     |
| SDA           | SDA     |

## SPI Communication with ADXL345

Here are [`spi_device_interface_config_t`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv429spi_device_interface_config_t) settings for [`spi_bus_add_device`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv418spi_bus_add_device17spi_host_device_tPK29spi_device_interface_config_tP19spi_device_handle_t),
- `command_bits` = `8`
- `mode` = `3` (CPOL=1, CPHA=1)
- `spics_io_num` = `5`

As a transaction with an ADXL345 always starts with a one-byte register address (command), I enabled a command (`command_bits=8`).

## ESP-IDF APIs

[`spi_bus_initialize`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv418spi_bus_initialize17spi_host_device_tPK16spi_bus_config_ti)

Initialization of an SPI bus.

[`spi_bus_add_device`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv418spi_bus_add_device17spi_host_device_tPK29spi_device_interface_config_tP19spi_device_handle_t)

Attachment of an ADXL345.

[`spi_device_polling_transmit`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv427spi_device_polling_transmit19spi_device_handle_tP17spi_transaction_t)

Transceiving data between an ESP32 and an ADXL345.

[`xTaskCreate`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/freertos.html#_CPPv411xTaskCreate14TaskFunction_tPCKcK8uint32_tPCv11UBaseType_tPC12TaskHandle_t)

Creation of a task that periodically samples acceleration from an ADXL345.

[`vTaskDelay`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/freertos.html#_CPPv410vTaskDelayK10TickType_t)

To sleep a task.