/**
 * @file spi_adxl345_main.c
 *
 * ADXL345 control via SPI.
 *
 * This program records acceleration with ADXL345.
 * Communication with ADXL345 is controlled via SPI.
 *
 * This file was tailored from the example of ESP-IDF `spi_master`.
 * https://github.com/espressif/esp-idf/tree/master/examples/peripherals/spi_master
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

/** @brief Uses SPI3 (VSPI). */
#define ADXL_HOST  VSPI_HOST
/** @brief DMA channel is not used. */
#define DMA_CHAN  0

#if ADXL_HOST == VSPI_HOST
/** @brief GPIO# for MISO. */
#define PIN_NUM_MISO  19
/** @brief GPIO# for MOSI. */
#define PIN_NUM_MOSI  23
/** @brief GPIO# for SCLK. */
#define PIN_NUM_CLK  18
/**
 * @brief GPIO# for CS.
 *
 * Also used as a strapping pin.
 */
#define PIN_NUM_CS  5
#elif ADXL_HOST == HSPI_HOST
/**
 * @brief GPIO# for MISO.
 *
 * Also used as MTDI.
 */
#define PIN_NUM_MISO 12 
/** @brief GPIO# for MOSI. */
#define PIN_NUM_MOSI  13
/** @brief GPIO# for SCLK. */
#define PIN_NUM_CLK  14
/** @brief GPIO# for CS. */
#define PIN_NUM_CS  15
#else
#error "unsupported SPI host"
#endif

/** @brief ADXL345 register read flag. */
#define ADXL345_REG_READ_FLAG  0x80u
/** @brief ADXL345 register multibyte flag. */
#define ADXL345_REG_MB_FLAG  0x40u
/** @brief ADXL345 register: DEVID. */
#define ADXL345_REG_DEVID  0x00u
/** @brief ADXL345 register: BW_RATE. */
#define ADXL345_REG_BW_RATE  0x2Cu
/** @brief ADXL345 register: POWER_CTL. */
#define ADXL345_REG_POWER_CTL  0x2Du
/** @brief ADXL345 register: DATAX0. */
#define ADXL345_REG_DATAX0  0x32u

/** @brief ADXL345 POWER_CTL flag: Measure. */
#define ADXL345_POWER_CTL_MEASURE  0x08u

/** @brief ADXL345 delay to update (200ms). */
#define ADXL345_UPDATE_DELAY  (200u / portTICK_PERIOD_MS)

/**
 * @brief Reads a given register from an ADXL345.
 *
 * In a 4-wire SPI transaction, transmission and receiving happen in parallel
 * as shown below.
 *
 * ```
 * Transmit: command --> t.tx_data[0]
 * Receive:  N/A     --> t.rx_data[0]
 *
 * where
 * command = address | ADXL345_READ_FLAG
 * and t is spi_transaction_t
 *   t.tx_data[1] = do not care
 *   t.rx_data[0] = register value
 * ```
 *
 * This function blocks until the transmission and receiving end.
 *
 * @param[in] spi
 *
 *   Handle of the ADXL345 from which the register is to be read.
 *
 * @param[in] address
 *
 *   Address of the register to be read.
 *
 * @return
 *
 *   Value of the register associated with `address`.
 */
static uint8_t adxl345_read (spi_device_handle_t spi, uint8_t address) {
    esp_err_t ret;
    spi_transaction_t trans = {
		.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA,
		.cmd = address | ADXL345_REG_READ_FLAG,
		.length = 8 // in bits
	};
    ret = spi_device_polling_transmit(spi, &trans);
    assert(ret == ESP_OK);
	return trans.rx_data[0];
}

/**
 * @brief Writes a given value in a specified register of an ADXL345.
 *
 * In a 4-wire SPI transaction, transmission and receiving happen in parallel
 * as shown below.
 *
 * ```
 * Transmit: address --> t.tx_data[0]
 * Receive:  N/A     --> t.rx_data[0]
 *
 * where t is spi_transaction_t and
 * t.tx_data[0] = value
 * t.rx_data[0] = do not care
 * ```
 *
 * This function blocks until the transmission and receiving end.
 *
 * @param[in] spi
 *
 *   Handle of the ADXL345 where the register value is to be set.
 *
 * @param[in] address
 *
 *   Address of the register to be written.
 *
 * @param[in] value
 *
 *   Value to be written to the register associated with `address`.
 */
static void adxl345_write (
		spi_device_handle_t spi,
		uint8_t address,
		uint8_t value)
{
	esp_err_t ret;
	spi_transaction_t trans = {
		.flags = SPI_TRANS_USE_RXDATA,
		.cmd = address,
		.tx_buffer = &value,
		.length = 8 // in bits
	};
	ret = spi_device_polling_transmit(spi, &trans);
	assert(ret == ESP_OK);
}

/**
 * @brief Reads the latest acceleration from the ADXL345.
 *
 * Undefined if `accs` does not point to a block smaller than
 * `sizeof(int16_t) * 3`
 *
 * @param[in] spi
 *
 *   Handle of the ADXL345 from which the latest acceleration is to be read.
 *
 * @param[out] accs
 *
 *   Buffer to receive acceleration.
 *   - `[0]`: x-acceleration
 *   - `[1]`: y-acceleration
 *   - `[2]`: z-acceleration
 */
static void adxl345_read_acceleration (spi_device_handle_t spi, int16_t* accs) {
	esp_err_t ret;
	uint8_t tx_buffer[3u * sizeof(uint16_t)]; // a dummy buffer
	spi_transaction_t trans = {
		.cmd = ADXL345_REG_READ_FLAG |
			ADXL345_REG_MB_FLAG |
			ADXL345_REG_DATAX0,
		.length = sizeof(tx_buffer) * 8, // in bits
		.tx_buffer = tx_buffer,
		.rx_buffer = accs
	};
	ret = spi_device_polling_transmit(spi, &trans);
	assert(ret == ESP_OK);
	// sample of each axis is represented in twos complement.
	// and as ESP32 is little endian, `accs` does not need swapping.
	// https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html#transactions-with-integers-other-than-uint8-t
}

/**
 * @brief Initializes the ADXL345.
 *
 * @param[in] spi
 *
 *   Handle of the ADXL345 to be initialized.
 */
static void adxl345_init (spi_device_handle_t spi) {
	uint8_t out;
    out = adxl345_read(spi, ADXL345_REG_DEVID);
    printf("DEVID: 0x%X\n", out);
	out = adxl345_read(spi, ADXL345_REG_BW_RATE);
	printf("BW_RATE: 0x%X\n", out);
}

/**
 * @brief Starts sampling of the ADXL345.
 *
 * This function blocks for 200ms after sending a start command.
 *
 * @param[in] spi
 *
 *   Handle of the ADXL345 to start.
 */
static void adxl345_start (spi_device_handle_t spi) {
	adxl345_write(spi, ADXL345_REG_POWER_CTL, ADXL345_POWER_CTL_MEASURE);
	vTaskDelay(ADXL345_UPDATE_DELAY);
}

/**
 * @brief Task that periodically reads accelerations.
 *
 * @param[in] pvParameters
 *
 *   (`spi_device_handle_t`) Handle to the ADXL345 from which the latest
 *   accleration is to be read.
 */
static void adxl345_read_acceleration_task (void* pvParameters) {
	int16_t accs[3];
	spi_device_handle_t spi = (spi_device_handle_t)pvParameters;
	while (1) {
		adxl345_read_acceleration(spi, accs);
		printf(
			"ax, ay, az: %d, %d, %d\n",
			(int)accs[0],
			(int)accs[1],
			(int)accs[2]);
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

void app_main (void) {
    esp_err_t ret;
    spi_device_handle_t spi;
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    spi_device_interface_config_t devcfg = {
		.clock_speed_hz = 1000000, // 1Mbps. nearest clock will be chosen.
        .mode = 3, // CPOL=1, CPHA=1
        .spics_io_num = PIN_NUM_CS,
		.command_bits = 8, // ADXL345 always takes 1+7 bit command (address).
        .queue_size = 1 // I do not know an appropriate size.
    };
    // initializes the SPI bus
    ret = spi_bus_initialize(ADXL_HOST, &buscfg, DMA_CHAN);
    ESP_ERROR_CHECK(ret);
    // attaches the ADXL to the SPI bus
    ret = spi_bus_add_device(ADXL_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
    // initializes the ADXL
    adxl345_init(spi);
	// starts sampling
	adxl345_start(spi);
	// periodically reads acceleration
	xTaskCreate(
		adxl345_read_acceleration_task,
		"adxl345_read_acceleration_task",
		2048u, // usStackDepth: I do not know an appropriate size.
		// DO NOT pass `&spi` because the task function would be executed
		// after this function finishes; i.e., &spi would be corrupted.
		(void*)spi, // pvParameters.
		5, // uxPriority: I do not know an appropriate priority.
		0); // pvCreatedTask
}
