#pragma once
/**
 * Xeus server class 
 * 
 * Creates and communicates the ZeroMQ channels
 * needs to communicate with the Jupyter server.
 * 
 * see 
 * 1) https://github.com/Slicer/SlicerJupyter/blob/master/JupyterKernel/xSlicerServer.cxx
 * 2.1) https://github.com/jupyter-xeus/xeus-qt/blob/main/src/xq_server.cpp
 * 2.2) https://github.com/jupyter-xeus/xeus-qt/blob/main/src/xq_qt_poller.cpp
 * 3) https://github.com/jupyter-xeus/xeus-zmq/blob/main/src/server/xserver_zmq_default.hpp
 * 
 * The xeus-qt example uses a separate poller thread
 * 
 * For simplicity the initial implementation is done with a QTimer (as in Slicer)
 * The timer interval is hardcoded to 10ms
 *
 * External dependencies are a xeus and xeus-zmq
 *
 *  - 
 * @authors B. van Lew
*/

#include <memory>

#include <xeus-zmq/xserver_zmq.hpp>
#include <xeus/xkernel_configuration.hpp>
#include <xeus/xeus_context.hpp>

#include <QObject>

class QTimer;

class XeusServer : public QObject, public xeus::xserver_zmq
{
public:

    Q_OBJECT

public: 
    XeusServer(xeus::xcontext& context,
               const xeus::xconfiguration& config,
               nl::json::error_handler_t eh);

    ~XeusServer();

protected:
    void start_impl(xeus::xpub_message message) override;
    void stop_impl() override;

private slots:

    void on_received_shell_msg(xeus::xmessage* msg);
    void on_received_control_msg(xeus::xmessage* msg);

private:
    QTimer* m_pollTimer = nullptr;
};

std::unique_ptr<xeus::xserver> make_XeusServer(xeus::xcontext& context,
    const xeus::xconfiguration& config,
    nl::json::error_handler_t eh);
