#include "XeusServer.h"
#include <QTimer>
#include <QDebug>

XeusServer::XeusServer(zmq::context_t& context,
                 const xeus::xconfiguration& config,
                 nl::json::error_handler_t eh) 
                 : xserver_zmq(context, config, eh)
{
  m_pollTimer = new QTimer();
  m_pollTimer->setInterval(10);
  QObject::connect(m_pollTimer, &QTimer::timeout, [=]() { poll(0); });
}

XeusServer::~XeusServer()
{
    m_pollTimer->stop();
    delete m_pollTimer;
}

void XeusServer::start_impl(xeus::xpub_message message)
{
    start_publisher_thread();
    start_heartbeat_thread();

    m_pollTimer->start();
    publish(std::move(message), xeus::channel::SHELL);

}

void XeusServer::stop_impl()
{
    xserver_zmq::stop_impl();
    m_pollTimer->stop();
    stop_channels(); // halt the communication channels
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // Slicer shutsdown the whole qSlicerApplication with exit(0)
    // when stop_impl is called 
}

std::unique_ptr<xeus::xserver> make_XeusServer(xeus::xcontext& context,
    const xeus::xconfiguration& config,
    nl::json::error_handler_t eh)
{
    qDebug() << "Server IP: " << config.m_ip.c_str();
    qDebug() << "Server Key: " << config.m_key.c_str();
    return std::make_unique<XeusServer>(context.get_wrapped_context<zmq::context_t>(), config, eh);
}