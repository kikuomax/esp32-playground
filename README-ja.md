# ESP32

言語: [English](README.md)/日本語

[ESP32](https://www.espressif.com/en/products/hardware/esp32/overview)で遊んでみよう。

## 準備

私は[ESP32 Feather](https://www.adafruit.com/product/3405)をゲットしました。Adafruit提供の開発ボード(ピンヘッダはんだ済)です。

私はMacOS (10.14.6)で作業しています。

## ESP-IDF

GitHubレポジトリ: https://github.com/espressif/esp-idf

ESP-IDFはESP32のプログラムをビルドしたりフラッシュに書き込んだりするためのツール群です。
FreeRTOSとAPIも含んでいます。

このレポジトリに[`esp-idf`](/esp-idf)としてESP-IDFのサブモジュールを作りました。

### ESP-IDFをインストールする

[マニュアル](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html)に従って、以下のステップを実施しました。

1. `esp-idf`ディレクトリに移動します。

    ```
    cd esp-idf
    ```

2. ESP-IDFをインストールします。

    ```
    ./install.sh
    ```

#### SSLエラー

すると次のファイルをダウンロードする際にSSLエラーが出ました。

```
https://github.com/espressif/openocd-esp32/releases/download/v0.10.0-esp32-20190313/openocd-esp32-macos-0.10.0-esp32-20190313.tar.gz
```

警告メッセージは

```
WARNING: Download failure [Errno socket error] [SSL: TLSV1_ALERT_PROTOCOL_VERSION] tlsv1 alert protocol version (_ssl.c:581)
```

これはPythonがTLS v1.0を使っており、GitHubサーバから拒絶されたことを意味します。
`install.sh`スクリプトはPython2をデフォルトで実施しますが、私は最近Python2を使っておらずアップデートもしていません。
ということで、[こちらのIssue](https://github.com/espressif/esp-idf/issues/4629)のようにPython3を使いたいと思いました。

(直接`idf_tools.py`を実行するような)侵襲性の高いアプローチはとりたくなかったので、virtual environmentを使うことにしました。
下記のステップを実施しました。

1. virtual environmentをPython3で作成します。

    ```
    python3 -m venv ./venv
    ```

2. virtual environmentをアクティベートします。

    ```
    source ./venv/bin/activate
    ```

3. `install.sh`を実行します。

    ```
    ./install.sh
    ```

#### 次のSSLエラー

今度は別のエラーが出ました。

```
WARNING: Download failure <urlopen error [SSL: CERTIFICATE_VERIFY_FAILED] certificate verify failed: unable to get local issuer certificate (_ssl.c:1076)>
```

こちらはPythonディストリビューションに付属の`Install Certificates`スクリプトを実行して証明書をインストールすることで解決しました。
この前に私のPython3を[公式のディストリビューション](https://www.python.org/downloads/release/python-376/)でアップデートしていました。

#### virtualenvモジュールエラー

さらに問題が発生しました。

[`virtualenv`](https://virtualenv.pypa.io/en/latest/)のインストールに失敗したのです。

最初は`idf_tools.py`の`virtualenv`を`venv`で置き換えて対応しました。
しかし、`venv`に対して[`virtualenv`には付加機能がある](https://virtualenv.pypa.io/en/latest/)ことに気づきました。
ということで元々の実装を尊重して`install.sh`を実行する前に`virtualenv`をインストールすることにしました。

```
pip install virtualenv
```

#### Python2

ん？Python2じゃないといけないのか？
とりあえずPython3で動いています。

## USB Driverをダウンロードする

ESP32 FeatherはUSB経由でPCと接続できます。

ESP32は適切なドライバをインストールするまで検出されませんでした。
`ls /dev/cu.*`コマンドを実行してもESP32はリストされません。

ひとつの疑問はどのUSBドライバをインストールすべきかということでした。
[このページ](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/using-with-arduino-ide)によると、ESP32 Featherには[CP210Xドライバ](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers)が必要ということが分かりました。

ドライバをインストールすると、`ls /dev/cu.*`コマンドは`/dev/cu.SLAB_USBtoUART`をリストしました。

## Ninjaをインストールする

私は[Ninja](https://ninja-build.org)をbrewでインストールしました。

```
brew install ninja
```

Ninjaなしでも[hello-worldサンプル](https://github.com/espressif/esp-idf/tree/master/examples/get-started/hello_world)のビルドに成功したので、Ninjaのインストールはオプションかもしれません。

## プロジェクトを記述する

[サンプル構造](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#example-project)。

プロジェクトをビルドする前に、以下のコマンドを実行しなければなりません。

```
source ./export.sh
```

## SPI

[このページ](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html)はESP32のSPIマスタドライバについて解説しています。
[こちら](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/spi_master)にSPIマスタのサンプルもあります。

### ESP32 Featherピンアウト

最初にESP32 Featherのピンアウトを理解する必要がありました。
[ESP32 Featherの配線図](https://learn.adafruit.com/assets/41630)と[このセクション](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#gpio-matrix-and-io-mux)によると、SPI関連のピンアウトは以下の通りです。

| Featherのラベル | GPIO# | SPI機能        |
|----------------|-------|---------------|
| SCK            | 5     | VSPICS0       |
| MO             | 18    | VSPICLK       |
| MI             | 19    | VSPIQ = MISO  |
| SDA            | 23    | VSPID = MOSI  |
| SCL            | 22    | VSPIWP (QSPI) |
| 21             | 21    | VSPIHD (QSPI) |

ESP32には4つのSPIバスがありますが我々が使えるのは、HSPI (SPI2)とVSPI (SPI3)の2つだけです。
上のテーブルはVSPIバス用のピンアウトを示しています。

#### HSPI接続

ドキュメントによると、HSPIバスには以下のピンが使えそうです。

| Featherのラベル | GPIO# | SPI機能        |
|----------------|-------|---------------|
| 15             | 15    | HSPICS0       |
| 14             | 14    | HSPICLK       |
| 12             | 12    | HSPIMISO      |
| 13 (LED点灯用?) | 13    | HSPIMOSI      |
| 不在 (GPIO2?)   | 2     | HSPIWP (QSPI) |
| A5             | 4     | HSPIHD (QSPI) |

ADXL345を上記の接続で試しましたが、GPIO12が接続されていると奇妙なフラッシュ書き込みエラーが出ました。

```
A fatal error occurred: Timed out waiting for packet content
CMake Error at run_cmd.cmake:14 (message):
  esptool.py failed
Call Stack (most recent call first):
  run_esptool.cmake:21 (include)
```

ESP32をモニタしようとすると、以下のメッセージが出ました。

```
rst:0x10 (RTCWDT_RTC_RESET),boot:0x33 (SPI_FAST_FLASH_BOOT)
flash read err, 1000
ets_main.c 371
```

[このページ](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)(ページでGPIO12を検索)に関連するコメントを見つけました。
コメントによると[このページ](https://github.com/espressif/esptool/wiki/ESP32-Boot-Mode-Selection)が説明しているとのことです。

以下、分かったことです。

GPIO12ピンはStrapping Pin (MTDI)と呼ばれています。
5つのStrapping Pinがあって、その電圧がブートモードとその他システムの設定に影響します。

私がESP32をモニタしようとした時に出くわしたエラーメッセージは[このセクション](https://github.com/espressif/esptool/wiki/ESP32-Boot-Mode-Selection#early-flash-read-error)に解説されています。

メッセージの`boot:0xXY`部分は以下のビットの組み合わせを表しており、どのStrapping PinがHighになっているかを示しています。
- `0x01`: GPIO5
- `0x02`: MTDO (GPIO15)
- `0x04`: GPIO4 (Strapping Pinとしてはリストされていない)
- `0x08`: GPIO2
- `0x10`: GPIO0
- `0x20`: MTDI (GPIO12)

`boot:0x33`は`GPIO5 | MTDO | GPIO0 | MTDI`を意味します。

VSPI接続の際は`boot:0x13`でした。つまり`GPIO5 | MTDO | GPIO0`です。

[解決策は面倒くさそう](https://esp32.com/viewtopic.php?t=5970)なので、とりあえずVSPIで行くことにしました。

#### GPIOマッピング

ESP32のGPIO番号は[GPIOマトリクス](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#gpio-matrix-and-io-mux)で仮想的にどの番号にでもマップできるようです。
しかしIO_MUXの定義によると、GPIOマトリクスを使わずにデフォルトのマッピングを使うとより効率的です。

以下は私が少しほじくった詳細です。
- [このコード](https://github.com/espressif/esp-idf/blob/c1d0daf36d0dca81c23c226001560edfa51c30ea/components/soc/soc/esp32s2/spi_periph.c)はデフォルトのSPIのマッピングを定義しています。
- [この関数](https://github.com/espressif/esp-idf/blob/4bfd0b961bea502a3bd2b0f64a933fbf87dc7349/components/driver/spi_common.c#L178-L192)は指定したGPIO番号がデフォルトとマッチするかをチェックしています。

### キーAPIs

[spi_bus_initialize](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv418spi_bus_initialize17spi_host_device_tPK16spi_bus_config_ti)

この関数を最初に呼びます。

[spi_bus_add_device](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv418spi_bus_add_device17spi_host_device_tPK29spi_device_interface_config_tP19spi_device_handle_t)

この関数でSPIデバイスを登録します。

[spi_device_polling_transmit](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv427spi_device_polling_transmit19spi_device_handle_tP17spi_transaction_t)

この関数はデータの転送と受信を行う便利な関数です。

### SPIの初期化

以下はVSPIバスを使う際の[`spi_bus_config_t`](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#_CPPv416spi_bus_config_t)の共通の設定です。
- `sclk_io_num`: `18`
- `mosi_io_num`: `23`
- `miso_io_num`: `19`
- `quadwp_io_num`: `-1` (無効)
- `quadhd_io_num`: `-1` (無効)

### ADXL345

[こちら](./adxl345)にADXL345と通信するサンプルコードがあります。