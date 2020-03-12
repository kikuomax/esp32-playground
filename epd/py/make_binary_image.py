#!/usr/bin/env python
# -*- coding: utf-8 -*-


import argparse
import logging
from matplotlib import image as mpimg


LOGGER = None


def pack_pixels(pixels, threshold):
    """Packs given pixels into a single value.

    :param pixels: pixels to be packed.
    :type pixels: numpy.ndarray

    :param threshold: threshold to determine a pixel is black or white.
    :type threshold: float

    :return: packed pixels.
    :rtype: int
    """
    packed = 0
    for p in pixels:
        packed <<= 1
        packed |= (p > threshold) and 1 or 0
    return packed


def convert_row(row, threshold):
    """Converts a given row into a list of packed bits.

    Each 8 pixels are packed into a single byte; i.e., 8 bit value.

    If the size of ``row`` is not a multiple of ``8``,
    zeros are appended so that the size becomes a multiple of ``8``.

    :param row: row to be converted.
    :type row: numpy.ndarray

    :param threshold: threshold to determine a pixel is black or white.
    :type threshold: float

    :return: list of packed pixels.
    :rtype: list
    """
    if (len(row) % 8) == 0:
        pixels_by_8 = row.reshape((int(len(row) / 8), 8))
    else:
        pixels_by_8 = row.resize((int(len(row) / 8), 8))
    return [pack_pixels(pixels, threshold) for pixels in pixels_by_8]


def convert_image(image_path, channel=0, threshold=0.5):
    """Converts a given image into a black-and-white binary image.

    The image is loaded by ``matplotlib.image.imread`` in an RGB format.

    ``channel``
    - ``0``: red
    - ``1``: green
    - ``2``: blue

    If a pixel value is greater than ``threshold``, the pixel is mapped to
    a white bit (``=1``).
    Otherwise it is mapped to a black bit (``=0``).

    :param image_path: path to an image to be converted into a binary image.
    :type image_path: str

    :param channel: channel to be converted into a binary image,
                    defaults to ``0``.
    :type dimension: int, optional

    :param threshold: threshold to determine a pixel is black or white,
                      defaults to ``0.5``.
    :type threshold: float, optional

    :return: list of rows composing the binary image.
    :rtype: list
    """
    LOGGER.info('loading image: ')
    image = mpimg.imread(image_path)
    LOGGER.info('converting')
    target = image[:, :, channel]
    return [convert_row(row, threshold) for row in target]


def print_row(row):
    """Prints a given row.

    :param row: row forming a binary image.
    :type row: list
    """
    print('\t%s,' % ', '.join('0x%02Xu' % b for b in row))


def print_rows(rows):
    """Prints given rows.

    :param rows: rows forming a binary image.
    :type rows: list
    """
    print('static const uint8_t IMAGE_DATA[] = {')
    for row in rows:
        print_row(row)
    print('};')


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    LOGGER = logging.getLogger(__name__)
    arg_parser = argparse.ArgumentParser(
        description='Convert image into a black and white binary image')
    arg_parser.add_argument(
        'image_path', metavar='IMAGE', type=str,
        help='path to an image to be converted')
    args = arg_parser.parse_args()
    LOGGER.info('IMAGE: %s', args.image_path)
    rows = convert_image(args.image_path)
    LOGGER.info('exporting')
    print_rows(rows)
