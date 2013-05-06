
#include "hw/GPIOInterruptThread.h"
#include "hw/HWInputButtonGPIO.h"
#include "util/Debug.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

GPIOInterruptThread::GPIOInterruptThread()
{
    m_epfd = epoll_create(2);

    if(m_epfd < 0)
        LOG_ERROR(Logger::Misc, "epoll_create has failed");

    m_bStop = false;

    pthread_mutex_init(&m_mutex, NULL);

    pthread_create(&m_thread, NULL, GPIOInterruptThread::run_internal, (void*)this);
}

GPIOInterruptThread::~GPIOInterruptThread()
{
    close(this->m_epfd);
    pthread_mutex_destroy(&m_mutex);
}

void GPIOInterruptThread::kill()
{
    // set stop to true, so the thread should exit soon
    pthread_mutex_lock(&m_mutex);
    m_bStop = true;
    pthread_mutex_unlock(&m_mutex);

    pthread_join(m_thread, NULL);
}

void GPIOInterruptThread::addGPIOInterrupt(HWInputButtonGPIO *hw)
{
    int fd = hw->getFileHandle();

    // invalid file handle
    if(fd < 0)
    {
        LOG_WARN(Logger::Misc, "Received invalid file handle");
        return;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.data.ptr = hw;
    event.events = EPOLLPRI;

    int ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &event);
    if(ret != 0)
        LOG_ERROR(Logger::Misc, "epoll_ctl has failed");
}

void GPIOInterruptThread::removeGPIOInterrupt(HWInputButtonGPIO *hw)
{
    int fd = hw->getFileHandle();

    // invalid file handle
    if(fd < 0)
    {
        LOG_WARN(Logger::Misc, "Received invalid file handle");
        return;
    }

    int ret = epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, NULL);
    if(ret != 0)
        LOG_ERROR(Logger::Misc, "epoll_ctl has failed");
}

void GPIOInterruptThread::run()
{
#define MAX_EVENTS  1
    struct epoll_event* events = new struct epoll_event[MAX_EVENTS];

    while(1)
    {
        // wait for 50 miliseconds
        int no = epoll_wait(this->m_epfd, events, MAX_EVENTS, 50);

        if(no != 0)
            ((HWInputButtonGPIO*)events[0].data.ptr)->handleInterrupt();

        pthread_mutex_lock(&m_mutex);
        // we want to stop
        if(m_bStop)
        {
            // we clean up and exit the loop
            delete events;

            pthread_mutex_unlock(&m_mutex);
            break;
        }

        pthread_mutex_unlock(&m_mutex);
    }
}

void* GPIOInterruptThread::run_internal(void* arg)
{
    GPIOInterruptThread* thread = (GPIOInterruptThread*)arg;
    thread->run();

    return NULL;
}
