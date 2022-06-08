#include <device.h>
#include <toolchain.h>

/* 1 : /soc/peripheral@40000000/clock@5000:
 * Direct Dependencies:
 *   - (/soc/interrupt-controller@e000e100)
 *   - (/soc/peripheral@40000000)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_peripheral_40000000_S_clock_5000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 2 : /soc/peripheral@40000000/gpio@842500:
 * Direct Dependencies:
 *   - (/soc/peripheral@40000000)
 * Supported:
 *   - (/gpio-reset)
 *   - (/buttons/button_0)
 *   - (/leds/led_1)
 *   - (/leds/led_2)
 *   - (/leds/led_3)
 *   - (/leds/led_4)
 *   - (/leds/led_5)
 *   - (/leds/led_6)
 *   - /soc/peripheral@40000000/spi@b000
 *   - (/soc/peripheral@40000000/i2c@a000/bh1749@38)
 *   - (/soc/peripheral@40000000/spi@b000/adxl362@0)
 *   - (/soc/peripheral@40000000/spi@b000/adxl372@1)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_peripheral_40000000_S_gpio_842500[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, 6, DEVICE_HANDLE_ENDS };

/* 3 : /cryptocell-sw:
 * Direct Dependencies:
 *   - (/)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_cryptocell_sw[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 4 : sysinit:
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_nrf91_socket[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 5 : /soc/peripheral@40000000/i2c@a000:
 * Direct Dependencies:
 *   - (/soc/interrupt-controller@e000e100)
 *   - (/soc/peripheral@40000000)
 * Supported:
 *   - (/soc/peripheral@40000000/i2c@a000/bh1749@38)
 *   - (/soc/peripheral@40000000/i2c@a000/bme680@76)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_peripheral_40000000_S_i2c_a000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 6 : /soc/peripheral@40000000/spi@b000:
 * Direct Dependencies:
 *   - (/soc/interrupt-controller@e000e100)
 *   - (/soc/peripheral@40000000)
 *   - /soc/peripheral@40000000/gpio@842500
 * Supported:
 *   - (/soc/peripheral@40000000/spi@b000/adxl362@0)
 *   - (/soc/peripheral@40000000/spi@b000/adxl372@1)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_peripheral_40000000_S_spi_b000[] = { 2, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };
