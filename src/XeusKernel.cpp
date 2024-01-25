#include "XeusKernel.h"
#include "XeusInterpreter.h"

#include <QStandardPaths>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <xeus/xhistory_manager.hpp>
#include <xeus-zmq/xserver_shell_main.hpp>
#include <xeus-python/xinterpreter.hpp>
#include <xeus-python/xdebugger.hpp>
#include <xeus-python/xpaths.hpp>
#include <memory>


XeusKernel::XeusKernel(std::string connection_filename) : m_connection_filename(connection_filename)
{

}

void XeusKernel::startKernel()
{
    auto context = xeus::make_context<zmq::context_t>();
    static const std::string executable(xpyt::get_python_path());
    nl::json debugger_config;
    debugger_config["python"] = executable;

    // Test without configuration file (default zmq ports)
    // xeus::xconfiguration config = xeus::load_configuration(m_connection_filename);
    std::unique_ptr<xeus::xinterpreter> interpreter = std::unique_ptr<xeus::xinterpreter>(new xpyt::interpreter());
    std::unique_ptr<xeus::xhistory_manager> hist = xeus::make_in_memory_history_manager();
    m_kernel = std::make_unique<xeus::xkernel>(
        // config,
        "ManiVaultStudio",
        std::move(context),
        std::move(interpreter),
        xeus::make_xserver_shell_main,
        std::move(hist),
        nullptr,
        xpyt::make_python_debugger,
        debugger_config
    );

    m_kernel->start();
}

void XeusKernel::stopKernel()
{

}

bool XeusKernel::startJupyterLabServer(QString noteBookDirectory)
{
    QString pythonExecutable = QStandardPaths::findExecutable("python");
    QStringList args;
    args <<  "-m" << "jupyter" << "lab" << "--notebook-dir" << noteBookDirectory;
    m_jupyterLabServer_process.setArguments(args);
    m_jupyterLabServer_process.start();
    auto success = m_jupyterLabServer_process.waitForStarted();

    return success;
}
