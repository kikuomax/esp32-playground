set(srcs
	"spi_adxl345_main.c")

idf_component_register(
	SRCS ${srcs}
	INCLUDE_DIRS ".")
