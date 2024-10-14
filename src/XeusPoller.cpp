#include "XeusPoller.h"

#include <iostream>

XeusWorkerThread::XeusWorkerThread(QObject* parent,
    poller_t poller)
    : QThread(parent)
    , m_poller(std::move(poller))
    , m_request_stop(false)
{
    std::cout << "starting thread" << std::endl;
}

void XeusWorkerThread::run()
{
    while (!m_request_stop.load())
    {
        auto msg = m_poller(10);
        if (msg)
        {
            if (msg.value().second == xeus::channel::SHELL)
            {
                // signals do not like the move semantics so
                // we need to put in a pointer and delete it on the receiving end
                xeus::xmessage* pmsg = new xeus::xmessage(std::move(msg.value().first));
                emit received_shell_msg_signal(pmsg);
            }
            else
            {
                // signals do not like the move semantics so
                // we need to put in a pointer and delete it on the receiving end
                xeus::xmessage* pmsg = new xeus::xmessage(std::move(msg.value().first));
                emit received_control_msg_signal(pmsg);
            }
        }
    }
}

void XeusWorkerThread::stop()
{
    m_request_stop = true;
}
