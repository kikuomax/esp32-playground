#ifndef _IMAGE_BUFFER_H
#define _IMAGE_BUFFER_H

/**
 * @file image_buffer.h
 *
 * Image buffer manipulation.
 */

#include <stdint.h>

#ifdef __cplusplus
externs "C" {
#endif

/**
 * @brief Image buffer.
 */
typedef struct image_buffer_t {
	/** @brief Memory block of this image buffer. */
	uint8_t* memory;
	/** @brief Width of the image buffer. */
	uint32_t width;
	/** @brief Height of the image buffer. */
	uint32_t height;
} image_buffer;

/**
 * @brief Initializer of an `::image_buffer`.
 *
 * Use this function macro to initialize an `::image_buffer`.
 *
 * It will cause undefined behavior if `_width` is not a multiple of `8`.
 *
 * @param[in] _memory
 *
 *   (`uint8_t*`) Memory block of the image buffer.
 *   You have to allocate a block as large as `_height * (_width / 8)`.
 *
 * @param[in] _width
 *
 *   (`uint32_t`) Width of the image buffer.
 *
 * @param[in] _height
 *
 *   (`uint32_t`) Height of the image buffer.
 *
 * @return
 *
 *   Initializer of an `::image_buffer`.
 */
#define image_buffer_initializer(_memory, _width, _height) \
{ \
	.memory = (_memory), \
	.width = (_width), \
	.height = (_height) \
}

/**
 * @brief Width of an `::image_buffer`.
 *
 * @param[in] buffer
 *
 *   (`const image_buffer*`) `image_buffer` whose width is to be obtained.
 *
 * @return
 *
 *   (`uint32_t`) Width of `buffer`.
 */
#define image_buffer_width(buffer)  (1 ? (buffer)->width : 0u)

/**
 * @brief Height of an `::image_buffer`.
 *
 * @param[in] buffer
 *
 *   (`const image_buffer*`) `::image_buffer` whose height is to be obtained.
 *
 * @return
 *
 *   (`uint32_t`) Height of `buffer`.
 */
#define image_buffer_height(buffer)  (1 ? (buffer)->height : 0u)

/**
 * @brief Beginning of the memory block of an `::image_buffer`.
 *
 * @param[in] buffer
 *
 *   (`const image_buffer*`)
 *   `::image_buffer` whose memory block is to be accessed.
 *
 * @return
 *
 *   (`uint8_t*`) Beginning of the memory block of `buffer`.
 */
#define image_buffer_begin(buffer)  (1 ? (buffer)->memory : 0u)

/**
 * @brief End of the memory block of an `::image_buffer`.
 *
 * @param[in] buffer
 *
 *   (`const image_buffer*`)
 *   `::image_buffer` whose memory block is to be accessed.
 *
 * @return
 *
 *   (`uint8_t*`) End of the memory block of `buffer`.
 */
#define image_buffer_end(buffer) \
	((buffer)->memory + ((buffer)->height * ((buffer)->width / 8u)))

/**
 * @brief Clears the entire `::image_buffer`.
 *
 * Fills all of the bits in `::image_buffer` with `1`.
 *
 * @param[in] buffer
 *
 *   `::image_buffer` to clear.
 */
void image_buffer_clear_all (const image_buffer* buffer);

/**
 * @brief Clears a range in an `::image_buffer`.
 *
 * Fills bits in a given area of `::image_buffer` with `1`.
 *
 * Will cause undefined behavior if `width` or `height` is negative.
 *
 * Will cause undefined behavior if `left` or `width` is not a multiple of `8`.
 *
 * @param[in] buffer
 *
 *   `::image_buffer` to clear.
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
 *   Width of the area to be cleared.
 *
 * @param[in] height
 *
 *   Height of the area to be cleared.
 */
void image_buffer_clear_range (
		const image_buffer* buffer,
		int left,
		int top,
		int width,
		int height);

/**
 * @brief Draws a given image in an `::image_buffer`.
 *
 * Will cause undefined behavior if `width` or `height` is negative.
 *
 * Will cause undefined behavior if `left` or `width` is not a multiple of `8`.
 *
 * @param[in] buffer
 *
 *   `::image_buffer` where a given image is to be drawn.
 *
 * @param[in] data
 *
 *   Pointer to image data to draw.
 *   Block must be as large as `height * (width / 8)`.
 *
 * @param[in] left
 *
 *   Left position of the image.
 *
 * @param[in] top
 *
 *   Top position of the image.
 *
 * @param[in] width
 *
 *   Width of the image.
 *
 * @param[in] height
 *
 *   Height of the image.
 */
void image_buffer_draw_image (
		const image_buffer* buffer,
		const uint8_t* data,
		int left,
		int top,
		int width,
		int height);

#ifdef __cplusplus
}
#endif

#endif
