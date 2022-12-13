/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
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
    if (!TaskBase::configureHook())
        return false;

    m_write_fds = openGPIOs(_w_configuration.get(), O_WRONLY, _sysfs_gpio_path.get());
    mCommand.states.resize(m_write_fds.size());
    m_read_fds = openGPIOs(_r_configuration.get(), O_RDONLY, _sysfs_gpio_path.get());
    mState.states.resize(m_read_fds.size());
    return true;
}
bool Task::startHook()
{
    if (!TaskBase::startHook())
        return false;

    auto now = base::Time::now();
    for (size_t i = 0; i < m_read_fds.size(); ++i) {
        bool value = readGPIO(m_read_fds[i]);
        mState.states[i].time = now;
        mState.states[i].data = value;
    }
    _r_states.write(mState);
    return true;
}
void Task::updateHook()
{
    while (_w_commands.read(mCommand, false) == RTT::NewData) {
        for (size_t i = 0; i < m_write_fds.size(); ++i)
            writeGPIO(m_write_fds[i], mCommand.states[i].data);
    }

    auto now = base::Time::now();
    bool hasUpdate = false;
    for (size_t i = 0; i < m_read_fds.size(); ++i) {
        int fd = m_read_fds[i];
        hasUpdate = true;
        bool value = readGPIO(fd);
        if (mState.states[i].data != value) {
            hasUpdate = true;
            mState.states[i].time = now;
            mState.states[i].data = value;
        }
    }
    if (hasUpdate) {
        mState.time = now;
        _r_states.write(mState);
    }
    TaskBase::updateHook();
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

namespace {
    struct CloseGuard {
        vector<int> fds;
        CloseGuard()
        {
        }
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

std::vector<int> Task::openGPIOs(Configuration const& config,
    int mode,
    string const& sysfs_root_path)
{
    CloseGuard guard;
    for (int id : config.ids) {
        string sysfs_path = sysfs_root_path + "/gpio" + to_string(id) + "/value";
        int fd = open(sysfs_path.c_str(), mode);
        if (fd == -1) {
            throw runtime_error("Failed to open " + sysfs_path + ": " + strerror(errno));
        }
        guard.push_back(fd);
    }
    return guard.release();
}

void Task::closeAll()
{
    for (int fd : m_write_fds) {
        close(fd);
    }
    m_write_fds.clear();

    for (int fd : m_read_fds) {
        close(fd);
    }
    m_read_fds.clear();
}

void Task::writeGPIO(int fd, bool value)
{
    lseek(fd, 0, SEEK_SET);
    char buf = value ? '1' : '0';
    int ret = write(fd, &buf, 1);
    if (ret != 1) {
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
    else
        return buf == '1';
}
