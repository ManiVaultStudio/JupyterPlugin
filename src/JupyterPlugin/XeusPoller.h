#pragma once

#include <atomic>
#include <functional>
#include <optional>
#include <utility>

#include <QThread>

#include <xeus/xserver.hpp>
#include <xeus/xmessage.hpp>

class XeusWorkerThread : public QThread
{
public:

    Q_OBJECT

public:

    using message_channel = std::pair<xeus::xmessage, xeus::channel>;
    using opt_message_channel = std::optional<message_channel>;
    using poller_t = std::function<opt_message_channel(long)>;

    XeusWorkerThread(QObject* parent,
        poller_t poller);

    void run();
    void stop();

private:

    poller_t m_poller;
    std::atomic<bool> m_request_stop;

signals:

    void received_shell_msg_signal(xeus::xmessage* msg);
    void received_control_msg_signal(xeus::xmessage* msg);
};

