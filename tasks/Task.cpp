/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace linux_gpios;
using namespace std;

Task::Task(string const& name)
    : TaskBase(name)
{
}

Task::Task(string const& name, RTT::ExecutionEngine* engine)
    : TaskBase(name, engine)
{
}

Task::~Task()
{
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

bool Task::configureHook()
{
    if (! TaskBase::configureHook()) {
        return false;
    }

    mOutput = openGPIOs<ConfiguredOutput>(_output_configuration.get(), O_WRONLY);
    mCommand.states.resize(mOutput.size());
    mInput = openGPIOs<ConfiguredInput>(_input_configuration.get(), O_RDONLY);
    mState.states.resize(mInput.size());
    return true;
}
bool Task::startHook()
{
    if (! TaskBase::startHook()) {
        return false;
    }

    auto now = base::Time::now();
    for (size_t i = 0; i < mInput.size(); ++i) {
        bool value = readGPIO(mInput[i].fd);
        mState.states[i].time = now;
        mState.states[i].data = value;
    }
    _input_pins.write(mState);
    return true;
}
void Task::updateHook()
{
    writeOutputs();
    readInputs();
    TaskBase::updateHook();
}

void Task::readInputs() {
    auto now = base::Time::now();
    bool hasUpdate = false;
    for (size_t i = 0; i < mInput.size(); ++i)
    {
        auto gpio = mInput[i];
        hasUpdate = true;
        bool value = readGPIO(gpio.fd);
        if (mState.states[i].data != value) {
            hasUpdate = true;
            mState.states[i].time = now;
            mState.states[i].data = value;
        }
    }
    if (hasUpdate)
    {
        mState.time = now;
        _input_pins.write(mState);
    }
}
void Task::writeOutputs() {
    auto flowState = _output_pins.read(mCommand, false);

    if (flowState == RTT::NoData) {
        handleOnNoData();
        return;
    }

    while (flowState == RTT::NewData)
    {
        for (size_t i = 0; i < mOutput.size(); ++i) {
            writeGPIO(mOutput[i].fd, mCommand.states[i].data);
        }
        flowState = _output_pins.read(mCommand, false);
    }
}

void Task::handleOnNoData(bool throw_on_error) {
    for (size_t i = 0; i < mOutput.size(); ++i) {
        if (mOutput[i].on_no_data == OutputConfiguration::USE_DEFAULT) {
            try {
                writeGPIO(mOutput[i].fd, mOutput[i].default_state);
            }
            catch (...) {
                if (throw_on_error) {
                    throw;
                }
            }
        }
    }
}

void Task::errorHook()
{
    TaskBase::errorHook();
}
void Task::stopHook()
{
    handleOnNoData();
    TaskBase::stopHook();
}
void Task::cleanupHook()
{
    handleOnNoData(false);
    closeAll();
    TaskBase::cleanupHook();
}

namespace
{
    struct CloseGuard
    {
        vector<int> fds;

        CloseGuard() {
        }

        ~CloseGuard()
        {
            for (int fd : fds) {
                close(fd);
            }
        }
        void push_back(int fd)
        {
            fds.push_back(fd);
        }
        vector<int> release()
        {
            vector<int> temp(move(fds));
            fds.clear();
            return temp;
        }
    };
}

template<typename T, typename C>
vector<T> Task::openGPIOs(vector<C> const& config, int mode)
{
    vector<T> result;

    CloseGuard guard;
    for (auto const& gpio : config) {
        int id = gpio.id;
        string sysfs_path = "/sys/class/gpio/gpio" + to_string(id) + "/value";
        int fd = open(sysfs_path.c_str(), mode);
        if (fd == -1)
        {
            throw runtime_error("Failed to open " + sysfs_path + ": "
                + strerror(errno));
        }
        guard.push_back(fd);

        result.push_back(T(fd, gpio));
    }
    guard.release();
    return result;
}

void Task::closeAll()
{
    for (auto gpio : mOutput) {
        close(gpio.fd);
    }
    mOutput.clear();

    for (auto gpio : mInput) {
        close(gpio.fd);
    }
    mInput.clear();
}

void Task::writeGPIO(int fd, bool value)
{
    lseek(fd, 0, SEEK_SET);
    char buf = value ? '1' : '0';
    int ret = write(fd, &buf, 1);
    if (ret != 1)
    {
        exception(IO_ERROR);
        throw std::runtime_error("failed to write GPIO status");
    }
}

bool Task::readGPIO(int fd)
{
    lseek(fd, 0, SEEK_SET);
    char buf;
    int ret = read(fd, &buf, 1);
    if (ret != 1) {
        exception(IO_ERROR);
        throw std::runtime_error("failed to read GPIO status");
    }
    else {
        return buf == '1';
    }
}
