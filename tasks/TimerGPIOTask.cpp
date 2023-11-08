/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "TimerGPIOTask.hpp"
#include "raw_io/Digital.hpp"
#include <chrono>
#include <thread>

using namespace linux_gpios;

TimerGPIOTask::TimerGPIOTask(std::string const& name)
    : TimerGPIOTaskBase(name)
{
}

TimerGPIOTask::~TimerGPIOTask()
{
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See TimerGPIOTask.hpp for more detailed
// documentation about them.

bool TimerGPIOTask::configureHook()
{
    if (!TimerGPIOTaskBase::configureHook())
        return false;
    return true;
}
bool TimerGPIOTask::startHook()
{
    if (!TimerGPIOTaskBase::startHook())
        return false;

    m_switch_timeout = _switch_timeout.get();
    m_timeout = _feedback_timeout.get();

    try {
        writeMessageAndCheckFeedback(m_switch_timeout, _set_state.get());
    }
    catch(...) {
        writeMessageAndCheckFeedback(m_switch_timeout, !_set_state.get());
        throw;
    }

    m_deadline = base::Time::now() + _duration.get();
    return true;
}
void TimerGPIOTask::updateHook()
{
    TimerGPIOTaskBase::updateHook();

    if (base::Time::now() < m_deadline) {
        writeMessageAndCheckFeedback(m_timeout, _set_state.get());
    }
    else {
        writeMessageAndCheckFeedback(m_timeout, !_set_state.get());
        stop();
    }
}
void TimerGPIOTask::errorHook()
{
    TimerGPIOTaskBase::errorHook();
}

void TimerGPIOTask::exceptionHook()
{
    try {
        writeMessageAndCheckFeedback(m_switch_timeout, !_set_state.get());
    }
    catch (const std::exception& e) {
    }
}

void TimerGPIOTask::stopHook()
{
    TimerGPIOTaskBase::stopHook();
}
void TimerGPIOTask::cleanupHook()
{
    TimerGPIOTaskBase::cleanupHook();
}

GPIOState TimerGPIOTask::createMessage(base::Time now, bool value)
{
    GPIOState output;
    output.time = now;
    output.states.push_back(raw_io::Digital(now, value));
    return output;
}

void TimerGPIOTask::writeMessageAndCheckFeedback(base::Time timeout, bool value)
{
    auto now = base::Time::now();
    auto deadline = now + timeout;
    GPIOState feedback;

    while (base::Time::now() <= deadline) {
        _gpio_state.write(createMessage(now, value));
        if (_feedback.read(feedback) == RTT::NewData) {
            if (feedback.states.size() > 1) {
                throw std::runtime_error("Feedback size is bigger than 1.");
            }
            if (feedback.states[0].data == value) {
                return;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    throw std::runtime_error("Feedback timeout.");
}