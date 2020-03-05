#ifndef linux_gpios_TYPES_HPP
#define linux_gpios_TYPES_HPP

#include <vector>
#include <base/Time.hpp>
#include <raw_io/Digital.hpp>

namespace linux_gpios {
    struct InputConfiguration {
        int id;
        InputConfiguration()
            : id(-1) {
        }
    };

    struct OutputConfiguration {
        enum NoDataAction {
            KEEP, USE_DEFAULT
        };

        int id;
        NoDataAction on_no_data;
        bool default_state;

        OutputConfiguration()
            : id(-1)
            , on_no_data(KEEP)
            , default_state(false) {
        }
    };

    struct GPIOState
    {
        base::Time time;
        std::vector<raw_io::Digital> states;
    };
}

#endif
