/* Generated from orogen/lib/orogen/templates/tasks/Task.hpp */

#ifndef LINUX_GPIOS_ESTOPLEDGPIOTASK_TASK_HPP
#define LINUX_GPIOS_ESTOPLEDGPIOTASK_TASK_HPP

#include "linux_gpios/EStopLedGPIOTaskBase.hpp"
#include <linux_gpios/PeriodicGPIO.hpp>

namespace linux_gpios {

    class EStopLedGPIOTask : public EStopLedGPIOTaskBase {
        friend class EStopLedGPIOTaskBase;

    private:
        std::unique_ptr<linux_gpios::PeriodicGPIO> m_transition_oscilator;
        std::unique_ptr<linux_gpios::PeriodicGPIO> m_unknown_state_oscilator;
        linux_gpios::GPIOState m_gpio_state;

        void setGPIOState(comms_wetpaint::SafetyStatus const& actual_status,
            comms_wetpaint::SafetyStatus const& desired_status);

    public:
        EStopLedGPIOTask(std::string const& name = "linux_gpios::EStopLedGPIOTask");
        ~EStopLedGPIOTask();
        bool configureHook();
        bool startHook();
        void updateHook();
        void errorHook();
        void stopHook();
        void cleanupHook();
    };
}

#endif
