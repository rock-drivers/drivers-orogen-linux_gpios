/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "EStopLedGPIOTask.hpp"
#include <memory>

using namespace linux_gpios;
using namespace std;
using namespace comms_wetpaint;

EStopLedGPIOTask::EStopLedGPIOTask(std::string const& name)
    : EStopLedGPIOTaskBase(name)
{
}

EStopLedGPIOTask::~EStopLedGPIOTask()
{
}

bool EStopLedGPIOTask::configureHook()
{
    if (!EStopLedGPIOTaskBase::configureHook())
        return false;
    m_transition_oscilator =
        make_unique<PeriodicGPIO>(_transition_blink_period.get(), false);
    m_unknown_state_oscilator =
        make_unique<PeriodicGPIO>(_unknown_state_blink_period.get(), false);
    m_gpio_state.states.resize(1);
    return true;
}

bool EStopLedGPIOTask::startHook()
{
    if (!EStopLedGPIOTaskBase::startHook())
        return false;
    return true;
}

void EStopLedGPIOTask::updateHook()
{
    EStopLedGPIOTaskBase::updateHook();

    comms_wetpaint::SafetyStatus actual_status;
    if (_actual_status.read(actual_status) == RTT::NoData) {
        return;
    }
    comms_wetpaint::SafetyStatus desired_status;
    if (_desired_status.read(desired_status) != RTT::NewData) {
        return;
    }

    setGPIOState(actual_status, desired_status);
    _gpio_state.write(m_gpio_state);
}

void EStopLedGPIOTask::setGPIOState(SafetyStatus const& actual_status,
    SafetyStatus const& desired_status)
{
    m_gpio_state.time = base::Time::now();
    m_gpio_state.states[0].time = base::Time::now();

    // Verify if the status is transitioning and is not unknown
    if (actual_status.safety_mode != desired_status.safety_mode &&
        actual_status.safety_mode != SafetyMode::UNKNOWN) {
        m_transition_oscilator->toggleGPIOValue();
        m_gpio_state.states[0].data = m_transition_oscilator->getGPIOValue();
        return;
    }

    switch (actual_status.safety_mode) {
        case SafetyMode::DISABLED:
            m_gpio_state.states[0].data = true;
            break;
        case SafetyMode::OPERATIONAL:
            m_gpio_state.states[0].data = false;
            break;
        case SafetyMode::EMERGENCY_BEHAVIOR:
            m_gpio_state.states[0].data = false;
            break;
        case SafetyMode::UNKNOWN:
            m_unknown_state_oscilator->toggleGPIOValue();
            m_gpio_state.states[0].data = m_unknown_state_oscilator->getGPIOValue();
            break;
        default:
            break;
    }
}

void EStopLedGPIOTask::errorHook()
{
    EStopLedGPIOTaskBase::errorHook();
}

void EStopLedGPIOTask::stopHook()
{
    EStopLedGPIOTaskBase::stopHook();
}

void EStopLedGPIOTask::cleanupHook()
{
    EStopLedGPIOTaskBase::cleanupHook();
}
