<<<<<<< HEAD
#include "event_manager.h"
#include "peripheral_module.h"
=======
<<<<<<< HEAD
// #include "event_manager.h"
=======
#include "event_manager.h"
#include "peripheral_module.h"
>>>>>>> master
>>>>>>> remotes/origin/kim

struct thingy_matrix_event {
        struct event_header header;
        // enum thingy_event_type type;

<<<<<<< HEAD
=======
<<<<<<< HEAD
//         /* Custom data fields. */
//         uint8_t data_array[3]; /* 2 first bytes are temperature ([integer],[decimal]) and last byte is relative humidity */

//         int32_t pressure_int; /*Integer part of the air pressure */
//         uint8_t pressure_float; /*Decimal part of the air pressure */
//         uint8_t battery_charge;
// };



// EVENT_TYPE_DECLARE(thingy_matrix_event);
=======
>>>>>>> remotes/origin/kim
        /* Custom data fields. */
        uint8_t thingy_matrix[THINGY_BUFFER_SIZE][11];
};

<<<<<<< HEAD
EVENT_TYPE_DECLARE(thingy_matrix_event);
=======
EVENT_TYPE_DECLARE(thingy_matrix_event);
>>>>>>> master
>>>>>>> remotes/origin/kim
