#pragma once
#include <QObject>
#include <QProcess>
#include <xeus/xkernel.hpp>
#include <xeus/xkernel_configuration.hpp>
#include <string>

/**
 * Xeus kernel class 
 * 
 * Hosts a Xeus kernel incorporating a Python interpreter and 0MQ 
 * communication context.
 * 
 * External dependencies are a 
 * 
 * see 
 * 1) https://github.com/jupyter-xeus/xeus-qt/blob/main/examples/main.cpp 
 *  Demonstrates:
 *  - launching a kernel and interpreter and 0MQ comms
 *  - 0MQ comms run is a Qt WorkerThread and responds to shell & control requests (need also to handle stdin requests)
 *  - 
 * @authors B. van Lew
*/
class XeusKernel : public QObject
{
    Q_OBJECT

public: 
    XeusKernel(std::string connection_filename);

    bool startJupyterLabServer(QString noteBookDirectory);
    void startKernel();
    void stopKernel();

private:
    std::unique_ptr<xeus::xkernel> m_kernel;
    QProcess m_jupyterLabServer_process;
    std::string m_connection_filename;
};