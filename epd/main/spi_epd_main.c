/**
 * @file spi_epd_main.c
 *
 * E-Paper Display (EPD) control via SPI.
 *
 * This program controls an EPD.
 * Communication with an EPD is controlled via SPI.
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
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "image_buffer.h"
#include "image_data.h"

/** @brief Uses SPI3 (VSPI). */
#define EPD_HOST  VSPI_HOST
/** @brief DMA channel is not used. */
#define DMA_CHAN  0
/**
 * @brief SPI mode.
 *
 * CPOL = CPHA = 0.
 */
#define SPI_MODE  0

#if EPD_HOST == VSPI_HOST
/**
 * @brief GPIO# for MISO.
 *
 * This pin is not used by an EPD.
 */
#define PIN_NUM_MISO  19
/** @brief GPIO# for MOSI. */
#define PIN_NUM_MOSI  23
/** @brief GPIO# for SCLK. */
#define PIN_NUM_CLK  18
/**
 * @brief GPIO# for CS.
 *
 * Also used as a strapping pin by an ESP32.
 */
#define PIN_NUM_CS  5
#elif E_INK_HOST == HSPI_HOST
/**
 * @brief GPIO# for MISO.
 *
 * Also used as MTDI.
 *
 * This pin is not used by an EPD.
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

/** @brief GPIO# for DC. */
#define PIN_NUM_BUSY  26
/** @brief GPIO# for RST.  */
#define PIN_NUM_RST  25
/** @brief GPIO# for BUSY */
#define PIN_NUM_DC  27

/** @brief Driver Output Control command. */
#define EPD_COMMAND_DRIVER_OUTPUT_CONTROL  0x01u
/** @brief Data Entry Mode command. */
#define EPD_COMMAND_DATA_ENTRY_MODE  0x11u
/** @brief Software Reset command. */
#define EPD_COMMAND_SW_RESET  0x12u
/** @brief Temperature Sensor Control command. */
#define EPD_COMMAND_TEMPERATURE_SENSOR_CONTROL  0x1Au
/** @brief Master Activation command. */
#define EPD_COMMAND_MASTER_ACTIVATION  0x20u
/** @brief Display Update Control command. */
#define EPD_COMMAND_DISPLAY_UPDATE_CONTROL_2  0x22u
/** @brief Write RAM (Black and White) command. */
#define EPD_COMMAND_WRITE_RAM_BW  0x24u
/**
 * @brief Border Waveform Control command.
 *
 * Undocumented in the datasheet.
 */
#define EPD_COMMAND_BORDER_WAVEFORM_CONTROL  0x3Cu
/** @brief RAM X Start / End Address command. */
#define EPD_COMMAND_RAM_X_START_END_ADDRESS  0x44u
/** @brief RAM Y Start / End Address command. */
#define EPD_COMMAND_RAM_Y_START_END_ADDRESS  0x45u
/** @brief RAM X Address command. */
#define EPD_COMMAND_RAM_X_ADDRESS  0x4Eu
/** @brief RAM Y Address command. */
#define EPD_COMMAND_RAM_Y_ADDRESS  0x4Fu

/** @brief Width of the EPD. */
#define EPD_WIDTH  200u
/** @brief Height of the EPD. */
#define EPD_HEIGHT  200u

/**
 * @brief Display Update Sequence `0xB1`.
 *
 * Represents the following sequence,
 * 1. Enable clock signal
 * 2. Load temperature value
 * 3. Load LUT with DISPLAY Mode 1
 * 4. Disable clock signal
 */
#define EPD_DISPLAY_UPDATE_SEQUENCE_TEMP_LUT_1  0xB1u

/**
 * @brief Display Update Sequence `0xB9`.
 *
 * Represents the following sequence,
 * 1. Enable clock signal
 * 2. Load temperature value
 * 3. Load LUT with DISPLAY Mode 2
 * 4. Disable clock signal
 */
#define EPD_DISPLAY_UPDATE_SEQUENCE_TEMP_LUT_2  0xB9u

/**
 * @brief Display Update Sequence `0xC7`.
 *
 * Represents the following sequence,
 * 1. Enable clock signal
 * 2. Enable Analog
 * 3. Display with DISPLAY Mode 1
 * 4. Disable Analog
 * 5. Disable OSC (clock signal?)
 */
#define EPD_DISPLAY_UPDATE_SEQUENCE_DISPLAY_1  0xC7u

/**
 * @brief Display Update Sequence `0xCF`.
 *
 * Represents the following sequence,
 * 1. Enable clock signal
 * 2. Enable Analog
 * 3. Display with DISPLAY Mode 2
 * 4. Disable Analog
 * 5. Disable OSC (clock signal?)
 */
#define EPD_DISPLAY_UPDATE_SEQUENCE_DISPLAY_2  0xCFu

// Define `EPD_SPECIFY_TEMPERATURE` if you want to manually set temperature.
// #define EPD_MANUAL_TEMPERATURE  1

/** @brief Data for Driver Output Control command. */
static const uint8_t DRIVER_OUTPUT_CONTROL_DATA[] = {
	// b[7..0]: lower bits of the number of lines
	0xC7u,
	// b[7..1]: 0
	// b[0]: high bit of the number of lines
	0x00u, // (0xC7 + 1) = 200 lines
	// b[2]: first output gate
	// b[1]: 0: normal, 1: interlace (even → odd)
	// b[0]: 0: G0 → G199, 1: G199 → G0
	0x00u
};

/** @brief Data for Data Entry Mode command. */
static const uint8_t DATA_ENTRY_MODE_DATA[] = {
	// b[2]: address counter direction. 0: x → y, 1: y → x
	// b[1..0]:
	// - 00(0): Y-decrement, X-decrement
	// - 01(1): Y-decrement, X-increment
	// - 10(2): Y-increment, X-decrement
	// - 11(3): Y-increment, X-increment
	0x03u
};

#ifdef  EPD_MANUAL_TEMPERATURE
/**
 * @brief Data for Temperature Sensor Control command.
 *
 * Represents 25℃.
 *
 * A fixed point number 8.4.
 * Two's complement.
 */
static const uint8_t DATA_TEMPERATURE_SENSOR_CONTROL_25_C[] = {
	// 25℃
	// b[7..0]: integer part
	25u,
	// b[7..4]: fraction
	0u
};
#endif

/** @brief Memory block for an `::image_buffer`. */
static uint8_t image_memory[EPD_HEIGHT * (EPD_WIDTH / 8u)];

/**
 * @brief Resets non-SPI GPIO pins.
 */
static void epd_configure_gpios (void) {
	esp_err_t ret;
	printf("epd_configure_gpios\n");
	ret = gpio_set_direction(PIN_NUM_BUSY, GPIO_MODE_INPUT);
	ESP_ERROR_CHECK(ret);
	ret = gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
	ESP_ERROR_CHECK(ret);
	ret = gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
	ESP_ERROR_CHECK(ret);
}

/**
 * @brief Resets an EPD.
 */
static void epd_reset (void) {
	esp_err_t ret;
	printf("epd_reset\n");
	ret = gpio_set_level(PIN_NUM_RST, 1u);
	ESP_ERROR_CHECK(ret);
	vTaskDelay(200 / portTICK_PERIOD_MS);
	ret = gpio_set_level(PIN_NUM_RST, 0u);
	ESP_ERROR_CHECK(ret);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	ret = gpio_set_level(PIN_NUM_RST, 1u);
	ESP_ERROR_CHECK(ret);
	vTaskDelay(200 / portTICK_PERIOD_MS);
}

/**
 * @brief Waits until the BUSY pin goes LOW.
 */
static void epd_wait_busy (void) {
	printf("epd_wait_busy\n");
	while (gpio_get_level(PIN_NUM_BUSY) == 1u) {
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
	printf("epd_wait_busy: done\n");
}

/**
 * @brief Sends a command to an EPD.
 *
 * @param[in] spi
 *
 *   Handle to an EPD.
 *
 * @param[in] command
 *
 *   Command to be sent.
 */
static void epd_send_command (spi_device_handle_t spi, uint8_t command) {
	esp_err_t ret;
	spi_transaction_t trans = {
		.length = 8u, // in bits
		.tx_buffer = &command
	};
	printf("epd_send_command: 0x%02X\n", (int)command);
	ret = gpio_set_level(PIN_NUM_DC, 0u);
	ESP_ERROR_CHECK(ret);
	ret = spi_device_polling_transmit(spi, &trans);
	ESP_ERROR_CHECK(ret);
}

/**
 * @brief Sends a single-byte data to an EPD.
 *
 * @param[in] spi
 *
 *   Handle to an EPD.
 *
 * @param[in] data
 *
 *   Data byte to be sent.
 */
static void epd_send_data_byte (spi_device_handle_t spi, uint8_t data) {
	esp_err_t ret;
	spi_transaction_t trans = {
		.length = 8u, // in bits
		.tx_buffer = &data
	};
	ret = gpio_set_level(PIN_NUM_DC, 1u);
	ESP_ERROR_CHECK(ret);
	ret = spi_device_polling_transmit(spi, &trans);
	ESP_ERROR_CHECK(ret);
}

/**
 * @brief Sends data to an EPD.
 *
 * It will cause undefined behavior if `data` is `NULL` or does not point to
 * a block smaller than `size`.
 *
 * @param[in] spi
 *
 *   Handle to an EPD.
 *
 * @param[in] data
 *
 *   Data to be sent.
 *
 * @param[in] size_in_bytes
 *
 *   Number of bytes to be sent.
 *   Subjects to the maximum size allowed in a single SPI transaction.
 */
static void epd_send_data (
	spi_device_handle_t spi,
	const uint8_t* data,
	size_t size_in_bytes)
{
	esp_err_t ret;
	spi_transaction_t trans = {
		.length = 8u * size_in_bytes, // in bits
		.tx_buffer = data
	};
	ret = gpio_set_level(PIN_NUM_DC, 1u);
	ESP_ERROR_CHECK(ret);
	ret = spi_device_polling_transmit(spi, &trans);
	ESP_ERROR_CHECK(ret);
}

/**
 * @brief Sets the x address range of an EPD.
 *
 * The current x address is reset to `start`.
 *
 * **Limitation:**
 * `start` and `end` are rounded to a largest multiple of 8 not exceeding it.
 *
 * @param[in] spi
 *
 *   Handle to an EPD.
 *
 * @param[in] start
 *
 *   Start x (**inclusive**).
 *
 * @param[in] end
 *
 *   End x (**inclusive**).
 */
static void epd_set_x_range (
	spi_device_handle_t spi,
	uint32_t start,
	uint32_t end)
{
	uint8_t data[] = {
		(uint8_t)(start / 8u),
		(uint8_t)(end / 8u)
	};
	printf("epd_set_x_range: %d, %d\n", (int)start, (int)end);
	epd_send_command(spi, EPD_COMMAND_RAM_X_START_END_ADDRESS);
	epd_send_data(spi, data, sizeof(data));
	epd_send_command(spi, EPD_COMMAND_RAM_X_ADDRESS);
	epd_send_data_byte(spi, data[0]);
}

/**
 * @brief Sets the y address range of an EPD.
 *
 * The current y address is reset to `start`.
 *
 * @param[in] spi
 *
 *   Handle to an EPD.
 *
 * @param[in] start
 *
 *   Start y (**inclusive**).
 *
 * @param[in] end
 *
 *   End y (**inclusive**).
 */
static void epd_set_y_range (
	spi_device_handle_t spi,
	uint32_t start,
	uint32_t end)
{
	uint8_t data[] = {
		// b[7..0]: lower bits of the start y
		(uint8_t)start,
		// b[0]: upper bit of the start y
		(uint8_t)((start >> 8) & 0x1u),
		// b[7..0]: lower bits of the end y
		(uint8_t)end,
		// b[0]: upper bit of the end y
		(uint8_t)((end >> 8) & 0x1u)
	};
	printf("epd_set_y_range: %d, %d\n", (int)start, (int)end);
	epd_send_command(spi, EPD_COMMAND_RAM_Y_START_END_ADDRESS);
	epd_send_data(spi, data, sizeof(data));
	epd_send_command(spi, EPD_COMMAND_RAM_Y_ADDRESS);
	epd_send_data(spi, data, 2u);
}

/**
 * @brief Sets the border filling of an EPD.
 *
 * Rendering is deferred until
 * `::epd_refresh_display_mode_1` or `::epd_refresh_display_mode_2` is called.
 *
 * @param[in] spi
 *
 *   Handle to an EPD.
 *
 * @param[in] fill
 *
 *   Border filling.
 *   - `0`: black
 *   - non-zero: white
 */
static void epd_set_border (spi_device_handle_t spi, uint8_t fill) {
	uint8_t data = (fill == 0u) ? 0u : 1u;
	printf("epd_set_border: 0x%02X\n", (int)fill);
	epd_send_command(spi, EPD_COMMAND_BORDER_WAVEFORM_CONTROL);
	epd_send_data_byte(spi, data);
}

/**
 * @brief Initializes an EPD.
 *
 * After calling this function, you need to enable a display mode with
 * either of the following functions,
 * - `::epd_enable_display_mode_1`
 * - `::epd_enable_display_mode_2`
 *
 * @param[in] spi
 *
 *   Handle to an EPD.
 */
static void epd_initialize (spi_device_handle_t spi) {
	printf("epd_initialize\n");
	epd_reset();
	// panel reset
	epd_wait_busy();
	epd_send_command(spi, EPD_COMMAND_SW_RESET);
	epd_wait_busy();
	// data output control
	epd_send_command(spi, EPD_COMMAND_DRIVER_OUTPUT_CONTROL);
	epd_send_data(
		spi,
		DRIVER_OUTPUT_CONTROL_DATA,
		sizeof(DRIVER_OUTPUT_CONTROL_DATA));
	// data entry mode
	epd_send_command(spi, EPD_COMMAND_DATA_ENTRY_MODE);
	epd_send_data(
		spi,
		DATA_ENTRY_MODE_DATA,
		sizeof(DATA_ENTRY_MODE_DATA));
	// RAM X start / end address
	epd_set_x_range(spi, 0u, EPD_WIDTH - 1u);
	// RAM Y start / end address
	epd_set_y_range(spi, 0u, EPD_HEIGHT - 1u);
	// Border Waveform Control
	epd_set_border(spi, 0u); // black border
	// without setting temperature, a display gets noisy
#ifdef  EPD_MANUAL_TEMPERATURE
	// Temperature Sensor Control
	// supposes the temperature is 25℃
	epd_send_command(spi, EPD_COMMAND_TEMPERATURE_SENSOR_CONTROL);
	epd_send_data(
		spi,
		DATA_TEMPERATURE_SENSOR_CONTROL_25_C,
		sizeof(DATA_TEMPERATURE_SENSOR_CONTROL_25_C));
#else
	// 0x18 is an unknown command
	// so far I guess that it turns automatic temperature sensing on
	// https://github.com/waveshare/e-Paper/blob/8973995e53cb78bac6d1f8a66c2d398c18392f71/RaspberryPi%26JetsonNano/c/lib/e-Paper/EPD_1in54_V2.c#L150-L151
	epd_send_command(spi, 0x18u);
	epd_send_data_byte(spi, 0x80u);
#endif
}

/**
 * @brief Enables the display mode 1 of an EPD.
 *
 * If you want to switch to the display mode 2,
 * you have to call `::epd_enable_display_mode_2`.
 *
 * @param[in] spi
 *
 *   Handle to an EPD.
 */
static void epd_enable_display_mode_1 (spi_device_handle_t spi) {
	printf("epd_enable_display_mode_1\n");
	epd_send_command(spi, EPD_COMMAND_DISPLAY_UPDATE_CONTROL_2);
	epd_send_data_byte(spi, EPD_DISPLAY_UPDATE_SEQUENCE_TEMP_LUT_1);
	epd_send_command(spi, EPD_COMMAND_MASTER_ACTIVATION);
	epd_wait_busy();
}

/**
 * @brief Enables the display mode 2 of an EPD.
 *
 * If you want to switch to the display mode 1,
 * you have to call `::epd_enable_display_mode_1`.
 *
 * @param[in] spi
 *
 *   Handle to an EPD.
 */
static void epd_enable_display_mode_2 (spi_device_handle_t spi) {
	printf("epd_enable_display_mode_2\n");
	epd_send_command(spi, EPD_COMMAND_DISPLAY_UPDATE_CONTROL_2);
	epd_send_data_byte(spi, EPD_DISPLAY_UPDATE_SEQUENCE_TEMP_LUT_2);
	epd_send_command(spi, EPD_COMMAND_MASTER_ACTIVATION);
	epd_wait_busy();
}

/**
 * @brief Refreshes an EPD with the display mode 1.
 *
 * Before calling this function,
 * you have to enable the display mode 1 with `::epd_enable_display_mode_1`.
 *
 * @param[in] spi
 *
 *   Handle to an EPD.
 */
static void epd_refresh_display_mode_1 (spi_device_handle_t spi) {
	printf("epd_refresh_display_mode_1\n");
	epd_send_command(spi, EPD_COMMAND_DISPLAY_UPDATE_CONTROL_2);
	epd_send_data_byte(spi, EPD_DISPLAY_UPDATE_SEQUENCE_DISPLAY_1);
	epd_send_command(spi, EPD_COMMAND_MASTER_ACTIVATION);
	epd_wait_busy();
}

/**
 * @brief Refreshes an EPD with the display mode 2.
 *
 * Before calling this function,
 * you have to enable the display mode 2 with `::epd_enable_display_mode_2`.
 *
 * @param[in] spi
 *
 *   Handle to an EPD.
 */
static void epd_refresh_display_mode_2 (spi_device_handle_t spi) {
	printf("epd_refresh_display_mode_2\n");
	epd_send_command(spi, EPD_COMMAND_DISPLAY_UPDATE_CONTROL_2);
	epd_send_data_byte(spi, EPD_DISPLAY_UPDATE_SEQUENCE_DISPLAY_2);
	epd_send_command(spi, EPD_COMMAND_MASTER_ACTIVATION);
	epd_wait_busy();
}

/**
 * @brief Clears a given range of an EPD.
 *
 * This function sets X and Y ranges to `[x, x + width - 1]` and
 * `[y, y + height - 1]` respectively.
 *
 * Will cause undefined behavior if `left` or `width` is not a multiple of `8`.
 *
 * @param[in] spi
 *
 *   Handle to an EPD.
 *
 * @param[in] left
 *
 *   Left position of the area to be cleared.
 *
 * @param[in] top
 *
 *   Top position of the area to be cleared.
 *
 * @param[in] width
 *
 *   Width of the are to be cleared.
 *
 * @param[in] height
 *
 *   Height of the are to be cleared.
 */
static void epd_clear_range (
		spi_device_handle_t spi,
		uint32_t left,
		uint32_t top,
		uint32_t width,
		uint32_t height)
{
	unsigned int i;
	unsigned int data_size = (unsigned int)(height * (width / 8u));
	assert((left % 8u) == 0u);
	assert((width % 8u) == 0u);
	printf(
		"epd_clear_range: x=%d, y=%d, w=%d, h=%d\n",
		(int)left,
		(int)top,
		(int)width,
		(int)height);
	epd_set_x_range(spi, left, left + (width - 1u));
	epd_set_y_range(spi, top, top + (height - 1u));
	epd_send_command(spi, EPD_COMMAND_WRITE_RAM_BW);
	for (i = 0u; i < data_size; ++i) {
		epd_send_data_byte(spi, 0xFFu);
	}
}

/**
 * @brief Clears all of the pixels of an EPD.
 *
 * This function resets x and y ranges to
 * `[0, EPD_WIDTH]` and `[0, EPD_HEIGHT]`.
 *
 * @param[in] spi
 *
 *   Handle to an EPD.
 */
static void epd_clear_all (spi_device_handle_t spi) {
	printf("epd_clear_all\n");
	epd_clear_range(spi, 0u, 0u, EPD_WIDTH, EPD_HEIGHT);
}

/**
 * @brief Draws a given image buffer on an EPD.
 *
 * This function sets the X and Y ranges to `[0, buffer->width-1]` and
 * `[0, buffer->height-1]` respectively.
 *
 * @param[in] spi
 *
 *   Handle to an EPD.
 *
 * @param[in] buffer
 *
 *   Image buffer to draw on the EPD.
 */
static void epd_draw_image_buffer (
		spi_device_handle_t spi,
		const image_buffer* buffer)
{
	const uint8_t* current;
	const uint8_t* end;
	printf("epd_draw_image_buffer\n");
	epd_set_x_range(spi, 0u, buffer->width - 1u);
	epd_set_y_range(spi, 0u, buffer->height - 1u);
	epd_send_command(spi, EPD_COMMAND_WRITE_RAM_BW);
	current = image_buffer_begin(buffer);
	end = image_buffer_end(buffer);
	while (current != end) {
		epd_send_data_byte(spi, *current);
		++current;
	}
}

void app_main (void) {
    esp_err_t ret;
    spi_device_handle_t spi;
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1, // disabled
        .quadhd_io_num = -1 // disabled
    };
    spi_device_interface_config_t devcfg = {
		.clock_speed_hz = 20000000, // 20MHz. nearest clock will be chosen.
        .mode = 0, // CPOL=0, CPHA=0
        .spics_io_num = PIN_NUM_CS, // CS is controlled by this program
        .queue_size = 1 // I do not know an appropriate size.
    };
	image_buffer buffer = image_buffer_initializer(
		image_memory,
		EPD_WIDTH,
		EPD_HEIGHT);
    // initializes the SPI bus
    ret = spi_bus_initialize(EPD_HOST, &buscfg, DMA_CHAN);
    ESP_ERROR_CHECK(ret);
    // attaches the ADXL to the SPI bus
    ret = spi_bus_add_device(EPD_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
	// configures GPIOs
	epd_configure_gpios();
	// initializes the display
	epd_initialize(spi);
	// enables and clears the display mode 2
	epd_enable_display_mode_2(spi);
	epd_clear_all(spi);
	epd_refresh_display_mode_2(spi);
	// displays a test image at different location
	// frame 1
	image_buffer_clear_all(&buffer);
	image_buffer_draw_image(&buffer, IMAGE_DATA, 64, 64, 64, 64);
	epd_draw_image_buffer(spi, &buffer);
	epd_refresh_display_mode_2(spi);
	// frame 2
	image_buffer_clear_range(&buffer, 64, 64, 64, 64);
	image_buffer_draw_image(&buffer, IMAGE_DATA, 128, 128, 64, 64);
	epd_draw_image_buffer(spi, &buffer);
	epd_refresh_display_mode_2(spi);
	// frame 3
	image_buffer_clear_range(&buffer, 128, 128, 64, 64);
	image_buffer_draw_image(&buffer, IMAGE_DATA, 0, 0, 64, 64);
	epd_draw_image_buffer(spi, &buffer);
	epd_refresh_display_mode_2(spi);
	// frame 4
	image_buffer_clear_range(&buffer, 0, 0, 64, 64);
	image_buffer_draw_image(&buffer, IMAGE_DATA, 96, 32, 64, 64);
	epd_draw_image_buffer(spi, &buffer);
	epd_refresh_display_mode_2(spi);
	// frame 5
	image_buffer_clear_range(&buffer, 96, 32, 64, 64);
	image_buffer_draw_image(&buffer, IMAGE_DATA, 24, 136, 64, 64);
	epd_draw_image_buffer(spi, &buffer);
	epd_refresh_display_mode_2(spi);
	// clears the display to prevent ghosting.
	// display mode 2 is not good for ghosting prevention.
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	epd_enable_display_mode_1(spi);
	epd_set_border(spi, 1u); // white border
	epd_clear_all(spi);
	epd_refresh_display_mode_1(spi);
}
