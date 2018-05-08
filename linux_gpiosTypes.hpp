#ifndef linux_gpios_TYPES_HPP
#define linux_gpios_TYPES_HPP

#include <vector>
#include <base/Time.hpp>
#include <raw_io/Digital.hpp>

namespace linux_gpios {
    struct Configuration
    {
        std::vector<int32_t> ids;
    };
    struct GPIOState
    {
        base::Time time;
        std::vector<raw_io::Digital> states;
    };
}

#endif
