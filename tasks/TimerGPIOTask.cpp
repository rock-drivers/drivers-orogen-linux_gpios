/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "TimerGPIOTask.hpp"
#include "raw_io/Digital.hpp"
#include <chrono>
#include <thread>

using namespace linux_gpios;
using namespace std;

TimerGPIOTask::TimerGPIOTask(std::string const& name)
    : TimerGPIOTaskBase(name)
{
    _deadline_report.set(base::Time::fromSeconds(1));
    _feedback_timeout.set(base::Time::fromSeconds(1));
    _switch_timeout.set(base::Time::fromSeconds(1));
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
    catch (...) {
        writeMessageAndCheckFeedback(m_switch_timeout, !_set_state.get());
        throw;
    }

    m_deadline = base::Time::now() + _duration.get();
    m_deadline_report = base::Time::now();

    return true;
}
void TimerGPIOTask::updateHook()
{
    TimerGPIOTaskBase::updateHook();

    auto now = base::Time::now();
    if (now < m_deadline) {
        writeMessageAndCheckFeedback(m_timeout, _set_state.get());
    }
    else {
        writeMessageAndCheckFeedback(m_timeout, !_set_state.get());
        stop();
    }

    if (now >= m_deadline_report) {
        _deadline.write(m_deadline);
        m_deadline_report = m_deadline_report + _deadline_report.get();
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

    bool received = false;
    while (base::Time::now() <= deadline) {
        _gpio_state.write(createMessage(now, value));
        if (_feedback.read(feedback) == RTT::NewData) {
            received = true;
            if (feedback.states.size() > 1) {
                throw std::runtime_error("Feedback size is bigger than 1.");
            }
            if (feedback.states[0].data == value) {
                return;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (received) {
        throw std::runtime_error(
            "Feedback timeout: received GPIO feedback, but none with value " +
            to_string(value) + " within " + to_string(timeout.toSeconds()) + " seconds");
    }
    else {
        throw std::runtime_error(
            "Feedback timeout: did not receive any GPIO feedback within " +
            to_string(timeout.toSeconds()) + " seconds");
    }
}