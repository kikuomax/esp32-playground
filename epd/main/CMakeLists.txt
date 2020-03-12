set(srcs
	"spi_epd_main.c"
	"image_buffer.c")

idf_component_register(
	SRCS ${srcs}
	INCLUDE_DIRS ".")
