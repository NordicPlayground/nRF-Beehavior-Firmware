#include <device.h>
#include <toolchain.h>

/* 1 : /soc/peripheral@50000000/clock@5000:
 * Direct Dependencies:
 *   - (/soc/interrupt-controller@e000e100)
 *   - (/soc/peripheral@50000000)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_peripheral_50000000_S_clock_5000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 2 : /soc/peripheral@50000000/gpio@842500:
 * Direct Dependencies:
 *   - (/soc/peripheral@50000000)
 * Supported:
 *   - (/gpio-reset)
 *   - (/buttons/button_0)
 *   - (/leds/led_1)
 *   - (/leds/led_2)
 *   - (/leds/led_3)
 *   - (/leds/led_4)
 *   - (/leds/led_5)
 *   - (/leds/led_6)
 *   - (/soc/peripheral@50000000/spi@b000)
 *   - (/soc/peripheral@50000000/i2c@a000/bh1749@38)
 *   - (/soc/peripheral@50000000/spi@b000/adxl362@0)
 *   - (/soc/peripheral@50000000/spi@b000/adxl372@1)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_peripheral_50000000_S_gpio_842500[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 3 : /soc/peripheral@50000000/uart@9000:
 * Direct Dependencies:
 *   - (/soc/interrupt-controller@e000e100)
 *   - (/soc/peripheral@50000000)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_peripheral_50000000_S_uart_9000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 4 : /soc/peripheral@50000000/uart@8000:
 * Direct Dependencies:
 *   - (/soc/interrupt-controller@e000e100)
 *   - (/soc/peripheral@50000000)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_peripheral_50000000_S_uart_8000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 5 : /soc/peripheral@50000000/flash-controller@39000:
 * Direct Dependencies:
 *   - (/soc/peripheral@50000000)
 * Supported:
 *   - (/soc/peripheral@50000000/flash-controller@39000/flash@0)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_peripheral_50000000_S_flash_controller_39000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };
