#pragma once

#include <xeus/xkernel.hpp>

#include <QString>

#include <memory>

/**
 * Xeus kernel class 
 * 
 * Hosts a Xeus kernel, which in turn hosts a Python interpreter 
 * and 0MQ communication context (zqm server).
 * 
 * see 
 * https://github.com/jupyter-xeus/xeus-qt/blob/main/examples/main.cpp 
 *
 *  - launches a kernel and interpreter and 0MQ comms
 *  - 0MQ comms run is a Qt WorkerThread and responds to shell & control requests (need also to handle stdin requests)
 *
 * External dependencies are xeus
 *
 * @authors B. van Lew
*/
class XeusKernel
{

public: 
    XeusKernel() = default;
    ~XeusKernel() = default;

    void startKernel(const QString& connection_path, const QString& pluginVersion = "");
    void stopKernel();

private:
    std::unique_ptr<xeus::xkernel> m_kernel = {};
};