#ifndef linux_gpios_TYPES_HPP
#define linux_gpios_TYPES_HPP

#include <base/Time.hpp>
#include <raw_io/Digital.hpp>
#include <vector>

namespace linux_gpios {
    struct ReadConfiguration {
        std::vector<int32_t> ids;
    };

    struct WriteConfiguration {
        std::vector<int32_t> ids;

        /** How long without input before the component writes default values */
        base::Time timeout;

        /** If non-empty, values to write to the GPIOs if it has no explicit command
         *
         * Default values are written if there was no command for `timeout`, or if
         * there is nothing connected to the command port
         */
        std::vector<uint8_t> defaults;
    };

    struct GPIOState {
        base::Time time;
        std::vector<raw_io::Digital> states;
    };
}

#endif
