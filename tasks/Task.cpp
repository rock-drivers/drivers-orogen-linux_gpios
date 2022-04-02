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
    if (! TaskBase::configureHook())
        return false;

    m_write_fds = openGPIOs(_w_configuration.get(), O_WRONLY);
    mIn.resize(m_write_fds.size());
    mCommand.states.resize(m_write_fds.size());

    m_read_fds = openGPIOs(_r_configuration.get(), O_RDONLY);
    mOut.resize(m_read_fds.size());
    mState.states.resize(m_read_fds.size());
    return true;
}
bool Task::startHook()
{
    if (! TaskBase::startHook())
        return false;

    handleOutput(true);
    return true;
}
void Task::updateHook()
{
    handleInput();
    handleOutput(false);
    TaskBase::updateHook();
}
void Task::handleInput() {
    while (_w_digital_io.read(mIn, false) == RTT::NewData)
    {
        for (size_t i = 0; i < m_write_fds.size(); ++i)
            writeGPIO(m_write_fds[i], mIn[i].data);
    }

    while (_w_commands.read(mCommand, false) == RTT::NewData)
    {
        for (size_t i = 0; i < m_write_fds.size(); ++i)
            writeGPIO(m_write_fds[i], mCommand.states[i].data);
    }
}
void Task::handleOutput(bool force) {
    auto now = base::Time::now();

    bool doOutput = false;
    for (size_t i = 0; i < m_read_fds.size(); ++i)
    {
        int fd = m_read_fds[i];
        bool value = readGPIO(fd);

        if (force || mOut[i].data != value) {
            doOutput = true;
            mOut[i].time = now;
            mOut[i].data = value;
            mState.states[i].time = now;
            mState.states[i].data = value;
        }
    }

    if (doOutput || !_output_on_change.get())
    {
        _r_digital_io.write(mOut);
        mState.time = now;
        _r_states.write(mState);
    }
}
void Task::errorHook()
{
    TaskBase::errorHook();
}
void Task::stopHook()
{
    TaskBase::stopHook();
}
void Task::cleanupHook()
{
    closeAll();
    TaskBase::cleanupHook();
}

namespace
{
    struct CloseGuard
    {
        vector<int> fds;
        CloseGuard() {}
        ~CloseGuard()
        {
            for (int fd : fds)
                close(fd);
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

std::vector<int> Task::openGPIOs(Configuration const& config, int mode)
{
    CloseGuard guard;
    for (int id : config.ids) {
        string sysfs_path = "/sys/class/gpio/gpio" + to_string(id) + "/value";
        int fd = open(sysfs_path.c_str(), mode);
        if (fd == -1)
        {
            throw runtime_error("Failed to open " + sysfs_path + ": "
                + strerror(errno));
        }
        guard.push_back(fd);
    }
    return guard.release();
}

void Task::closeAll()
{
    for (int fd : m_write_fds)
    {
        close(fd);
    }
    m_write_fds.clear();

    for (int fd : m_read_fds)
    {
        close(fd);
    }
    m_read_fds.clear();
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
    if (ret != 1)
    {
        exception(IO_ERROR);
        throw std::runtime_error("failed to read GPIO status");
    }
    else
        return buf == '1';
}
