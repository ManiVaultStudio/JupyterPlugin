#pragma once

#include <xeus/xkernel.hpp>

#include <QString>

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
class XeusKernel
{

public: 
    XeusKernel();
    virtual ~XeusKernel() = default;

    void startKernel(const QString& connection_path, const QString& pluginVersion = "");
    void stopKernel();


private:
    xeus::xkernel *m_kernel = nullptr;
};