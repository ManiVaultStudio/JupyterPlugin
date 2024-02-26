#pragma once
#include <QObject>
#include <QProcess>
#include <xeus/xkernel.hpp>
#include <xeus/xkernel_configuration.hpp>
#include <string>
#include <nlohmann/json.hpp>

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
    XeusKernel();
    virtual ~XeusKernel()=default;

    bool startJupyterLabServer(QString noteBookDirectory);
    void startKernel(const QString& connection_path);
    void stopKernel();

private:
    void onReadyStandardOutput();
    void onReadyStandardError();

private:
    xeus::xkernel *m_kernel;
    QProcess m_jupyterLabServer_process;
};