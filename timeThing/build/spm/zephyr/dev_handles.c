#include <device.h>
#include <toolchain.h>

/* 1 : /soc/peripheral@50000000/clock@5000:
 * Direct Dependencies:
 *   - (/soc/interrupt-controller@e000e100)
 *   - (/soc/peripheral@50000000)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_peripheral_50000000_S_clock_5000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 2 : /soc/crypto@50840000:
 * Direct Dependencies:
 *   - (/soc)
 * Supported:
 *   - (/soc/crypto@50840000/crypto@50841000)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_crypto_50840000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

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

/* 5 : /soc/peripheral@50000000/i2c@a000:
 * Direct Dependencies:
 *   - (/soc/interrupt-controller@e000e100)
 *   - (/soc/peripheral@50000000)
 * Supported:
 *   - (/soc/peripheral@50000000/i2c@a000/bh1749@38)
 *   - (/soc/peripheral@50000000/i2c@a000/bme680@76)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_peripheral_50000000_S_i2c_a000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };
