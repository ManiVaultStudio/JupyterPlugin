#pragma once
/**
 * Xeus server class 
 * 
 * Creates and communicates the ZeroMQ channels
 * needs to communicate with the Jupyter server.
 * 
 * External dependencies are a xeus-zmq and zmq
 * 
 * see 
 * 1) https://github.com/Slicer/SlicerJupyter/blob/master/JupyterKernel/xSlicerServer.cxx
 * 2.1) https://github.com/jupyter-xeus/xeus-qt/blob/main/src/xq_server.cpp
 * 2.2) https://github.com/jupyter-xeus/xeus-qt/blob/main/src/xq_qt_poller.cpp
 * 
 * Slicer uses a QTimer to manage polling. 
 * The xeus-qt example uses a separate poller thread
 * 
 * For simplicity the initial implementation is done with the QTimer solution as Slicer
 * The timer interval is hardcoded to 10ms
 *  - 
 * @authors B. van Lew
*/

// xeus includes
#include <xeus-zmq/xserver_zmq.hpp>
#include <xeus/xkernel_configuration.hpp>
#include <zmq.hpp>
#include "XeusPoller.h"

class QTimer;

class XeusServer : public QObject, public xeus::xserver_zmq
{
public:

    Q_OBJECT

public: 
    XeusServer(zmq::context_t& context,
               const xeus::xconfiguration& config,
               nl::json::error_handler_t eh);

    virtual ~XeusServer();

    // The Slicer implementation gives the option
    // to get and change the poll interval.
    // May add this for advanced experimentation.


protected:
    void start_impl(xeus::xpub_message message) override;
    void stop_impl() override;

    QTimer* m_pollTimer;

private slots:

    void on_received_shell_msg(xeus::xmessage* msg);
    void on_received_control_msg(xeus::xmessage* msg);
};

std::unique_ptr<xeus::xserver> make_XeusServer(xeus::xcontext& context,
    const xeus::xconfiguration& config,
    nl::json::error_handler_t eh);