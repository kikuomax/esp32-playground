# ADXL345

言語: [English](README.md)/日本語

ADXL345は３軸加速度センサです。

[データシート](https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf)。

## 開発ボード

私は[こちら](http://akizukidenshi.com/catalog/g/gM-06724/)をゲットしました。

## 接続

こちらは私のESP32 FeatherとADXL345の接続です。

| Featherラベル | ADXL345 |
|--------------|---------|
| 3V           | VDD     |
| GND          | GND     |
| SCK          | CS      |
| MO           | SCL     |
| MI           | SDO     |
| SDA          | SDA     |

## ADXL345とのSPI通信

こちらは¥[`spi_bus_add_device`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv418spi_bus_add_device17spi_host_device_tPK29spi_device_interface_config_tP19spi_device_handle_t)に渡す[`spi_device_interface_config_t`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv429spi_device_interface_config_t)の設定です。
- `command_bits` = `8`
- `mode` = `3` (CPOL=1, CPHA=1)
- `spics_io_num` = `5`

ADXL345とのトランザクションは必ず1バイトのレジスタアドレス(コマンド)から始まるので、コマンドを有効にしました(`command_bits=8`)。

## ESP-IDF API

[`spi_bus_initialize`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv418spi_bus_initialize17spi_host_device_tPK16spi_bus_config_ti)

SPIバスを初期化します。

[`spi_bus_add_device`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv418spi_bus_add_device17spi_host_device_tPK29spi_device_interface_config_tP19spi_device_handle_t)

ADXL345を追加します。

[`spi_device_polling_transmit`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv427spi_device_polling_transmit19spi_device_handle_tP17spi_transaction_t)

ESP32とADXL345の間のデータを送受信します。

[`xTaskCreate`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/freertos.html#_CPPv411xTaskCreate14TaskFunction_tPCKcK8uint32_tPCv11UBaseType_tPC12TaskHandle_t)

ADXL345から定期的に加速度をサンプリングするタスクを作成します。

[`vTaskDelay`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/freertos.html#_CPPv410vTaskDelayK10TickType_t)

タスクをスリープさせます。