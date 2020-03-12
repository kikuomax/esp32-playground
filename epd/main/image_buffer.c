/**
 * @file image_buffer.c
 *
 * Implementation of image buffer.
 */

#include "image_buffer.h"
#include "utils.h"

#include <assert.h>
#include <string.h>

void image_buffer_clear_all (const image_buffer* buffer) {
	memset(buffer->memory, 0xFF, buffer->height * (buffer->width / 8u));
}

void image_buffer_clear_range (
		const image_buffer* buffer,
		int left,
		int top,
		int width,
		int height)
{
	uint8_t* dest;
	uint8_t* dest_next;
	int y;
	int right = left + width;
	int bottom = top + height;
	int row_bytes = (int)image_buffer_width(buffer) / 8;
	assert(width >= 0);
	assert(height >= 0);
	assert((left % 8) == 0);
	assert((width % 8) == 0);
	left = MAX(0, left);
	top = MAX(0, top);
	right = MIN(right, (int)image_buffer_width(buffer));
	bottom = MIN(bottom, (int)image_buffer_height(buffer));
	if ((left >= right) || (top >= bottom)) {
		return;
	}
	width = right - left;
	height = bottom - top;
	dest_next = image_buffer_begin(buffer) + (top * row_bytes) + (left / 8);
	for (y = 0; y < height; ++y) {
		dest = dest_next;
		dest_next += row_bytes;
		memset(dest, 0xFF, width / 8);
	}
}

void image_buffer_draw_image (
		const image_buffer* buffer,
		const uint8_t* data,
		int left,
		int top,
		int width,
		int height)
{
	const uint8_t* src;
	const uint8_t* src_next;
	uint8_t* dest;
	uint8_t* dest_next;
	int y;
	int src_scan_size = (int)width / 8;
	int dest_scan_size = (int)image_buffer_width(buffer) / 8;
	int right = left + width;
	int bottom = top + height;
	assert(width >= 0);
	assert(height >= 0);
	assert((left % 8) == 0);
	assert((width % 8) == 0);
	dest_next = image_buffer_begin(buffer) +
		(top * dest_scan_size) +
		(left / 8);
	src_next = data;
	if (left < 0) {
		src_next += (-left) / 8;
		dest_next += (-left) / 8;
		width += left;
	}
	if (top < 0) {
		src_next += (-top) * src_scan_size;
		dest_next += (-top) * dest_scan_size;
		height += top;
	}
	if (right > (int)image_buffer_width(buffer)) {
		width -= right - (int)image_buffer_width(buffer);
	}
	if (bottom > (int)image_buffer_height(buffer)) {
		height -= bottom - (int)image_buffer_height(buffer);
	}
	if ((width <= 0) || (height <= 0)) {
		return;
	}
	for (y = 0; y < height; ++y) {
		src = src_next;
		src_next += src_scan_size;
		dest = dest_next;
		dest_next += dest_scan_size;
		memcpy(dest, src, width / 8);
	}
}

