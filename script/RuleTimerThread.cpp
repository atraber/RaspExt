
#include "script/RuleTimerThread.h"
#include "script/Rule.h"
#include "util/Debug.h"

#include <signal.h>


static void dummy_handler(int)
{
    // nothing here
}

RuleTimerThread::RuleTimerThread()
{
    m_bStop = false;
    m_bPaused = false;
    m_thread = 0;

    // specify a dummy handler for SIGUSR1
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = dummy_handler;
    sigaction(SIGUSR1, &sa, NULL);
}

RuleTimerThread::~RuleTimerThread()
{
    if(m_thread != 0)
        this->kill();
}

/**
 * @brief RuleTimerThread::start starts this thread
 */
void RuleTimerThread::start()
{
    pi_assert(m_thread == 0);

    m_bStop = false;

    // create and run thread
    pthread_create(&m_thread, NULL, RuleTimerThread::run_internal, (void*)this);
}

/**
 * @brief RuleTimerThread::kill kills this thread. If this thread is deleted it is killed automatically.
 */
void RuleTimerThread::kill()
{
    // only do something if this thread is really started
    if(m_thread == 0)
        return;

    // set stop to true, so the thread should exit soon
    m_mutex.lock();
    m_bStop = true;
    m_mutex.unlock();

    // deliver signal to thread to wake it up
    pthread_kill(m_thread, SIGUSR1);

    pthread_join(m_thread, NULL);
    m_thread = 0;
}

/**
 * @brief RuleTimerThread::clear is used to clear the queue of RuleTimerThread.
 * All queued operations are deleted immediatly. The thread must be stopped to do this!
 */
void RuleTimerThread::clear()
{
    pi_assert(m_thread == 0);

    m_mutex.lock();
    while(!m_queue.empty()) m_queue.pop();
    m_mutex.unlock();
}

/**
 * @brief RuleTimerThread::addRule adds a rule to this thread which should continue to be executed as soon as waitMs miliseconds have elapsed.
 * @param rule the rule which should be executed
 * @param start the last action which was executed
 * @param waitMs the time in miliseconds for which we should wait
 */
void RuleTimerThread::addRule(Rule* rule, unsigned int start, unsigned int waitMs)
{
    Element element;
    element.rule = rule;
    element.start = start;

    // set timer
    clock_gettime(CLOCK_MONOTONIC, &element.time);
    element.time = timspecAddMiliseconds(element.time, waitMs);

    // add element to queue
    m_mutex.lock();
    m_queue.push(element);
    m_mutex.unlock();

    // deliver signal to thread to wake it up
    // funny to use kill to wake a thread :-D
    pthread_kill(m_thread, SIGUSR1);
}

/**
 * @brief RuleTimerThread::continueTimer continues a timer which was paused by RuleTimerThread::pauseTimer before.
 */
void RuleTimerThread::continueTimer()
{
    if(m_bPaused)
    {
        timespec timeDiff;
        clock_gettime(CLOCK_MONOTONIC, &timeDiff);
        timeDiff = timespecSub(timeDiff, m_pausedTime);

        for(PriorityQueue<Element>::iterator it = m_queue.begin(); it != m_queue.end(); it++)
        {
            // as an add does not modify the internal order of the heap, we do not have to rebuild the heap
            (*it).time = timespecAdd((*it).time, timeDiff);
        }

        this->start();
        m_bPaused = false;
    }
}

/**
 * @brief RuleTimerThread::pauseTimer pauses a timer. It can later be continued by RuleTimerThread::continueTimer
 */
void RuleTimerThread::pauseTimer()
{
    if(!m_bPaused)
    {
        this->kill();

        m_bPaused = true;
        clock_gettime(CLOCK_MONOTONIC, &m_pausedTime);
    }
}

void RuleTimerThread::run()
{
    timespec currentTime;
    timespec waitTime;
    while(true)
    {
        m_mutex.lock();

        if(m_bStop)
        {
            // we want to exit the while loop
            m_mutex.unlock();
            break;
        }

        // check if queue is empty, if yes then sleep
        if( m_queue.empty() )
        {
            m_mutex.unlock();

            // sleep for 100ms and wake up if we receive a signal
            timespec sleep;
            sleep.tv_sec = 0;
            sleep.tv_nsec = 100000000;

            if(nanosleep(&sleep, NULL)  == -1)
            {
                //LOG_WARN(Logger::Script, "Interrupted by signal");
            }
            continue;
        }

        // get element from queue
        Element element = m_queue.top();

        m_mutex.unlock();


        // get current time
        clock_gettime(CLOCK_MONOTONIC, &currentTime);

        // if this is true we have to sleep first
        if( timespecGreaterThan(element.time, currentTime) )
        {
            // calculate time difference for nanosleep
            waitTime = timespecSub(element.time, currentTime);

            // check for signals during sleep
            if( nanosleep(&waitTime, NULL) < 0)
            {
                // we have been interrupted by a signal, normally this happens to wake this thread
                //LOG_WARN(Logger::Script, "Interrupted by signal");

                // go back to start and check queue
                continue;
            }
        }

        // now we should do something as the timer has expired
        element.rule->executeActions(element.start);


        m_mutex.lock();

        // now we have to remove the current element from the heap
        m_queue.pop();

        m_mutex.unlock();
    }
}

void* RuleTimerThread::run_internal(void* arg)
{
    RuleTimerThread* thread = (RuleTimerThread*)arg;
    thread->run();

    return NULL;
}
