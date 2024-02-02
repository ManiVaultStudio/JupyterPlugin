#include "XeusKernel.h"
#include "XeusInterpreter.h"

#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
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
    connect(&m_jupyterLabServer_process, &QProcess::readyReadStandardOutput, this, &XeusKernel::onReadyStandardOutput);
    connect(&m_jupyterLabServer_process, &QProcess::readyReadStandardError, this, &XeusKernel::onReadyStandardError);
}

void XeusKernel::startKernel()
{
    auto searchPath = QStringList(QCoreApplication::applicationDirPath() + "/python");
    QString pythonExecutable = QStandardPaths::findExecutable("python", searchPath);
    static const std::string executable(pythonExecutable.toLatin1());
    nl::json debugger_config;
    debugger_config["python"] = executable;

    //// Test without configuration file (default zmq ports)
    //// xeus::xconfiguration config = xeus::load_configuration(m_connection_filename);
    auto context = xeus::make_context<zmq::context_t>();
    std::unique_ptr<XeusInterpreter> interpreter = std::unique_ptr<XeusInterpreter>(new XeusInterpreter());
    std::unique_ptr<xeus::xhistory_manager> hist = xeus::make_in_memory_history_manager();
    m_kernel = new xeus::xkernel(
        xeus::get_user_name(),
        std::move(context),
        std::move(interpreter),
        xeus::make_xserver_shell_main
        //std::move(hist),
        //xeus::make_console_logger(xeus::xlogger::msg_type, xeus::make_file_logger(xeus::xlogger::content, "xeus.log"))
        //xpyt::make_python_debugger,
        //debugger_config
    );
    const auto& config = m_kernel->get_config();
    qInfo() << "Xeus Kernel settings";
    qInfo() << "transport protocol: " << config.m_transport.c_str();
    qInfo() << "IP address: " << config.m_ip.c_str();
    qInfo() << "control port: " << config.m_control_port.c_str();
    qInfo() << "shell port: " << config.m_shell_port.c_str();
    qInfo() << "stdin port: " << config.m_stdin_port.c_str();
    qInfo() << "iopub port: " << config.m_iopub_port.c_str();
    qInfo() << "hb port: " << config.m_hb_port.c_str();
    qInfo() << "signature scheme: " << config.m_signature_scheme.c_str();
    qInfo() << "connection key: " << config.m_key.c_str();

    m_kernel->start();
}

void XeusKernel::stopKernel()
{

}

bool XeusKernel::startJupyterLabServer(QString noteBookDirectory)
{
    //auto searchPath = QStringList(QCoreApplication::applicationDirPath() + "/python");
    auto vbsExe = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/python/runjupyter.vbs");
    //QString pythonExecutable = QStandardPaths::findExecutable("pythonw", searchPath);
    QStringList args;
    args << "/nologo" << vbsExe;
    qInfo() << "Executing: " << vbsExe;
    //args << "-m" << "jupyter" << "lab" << "--no-browser";
    //QString execString = QStandardPaths::findExecutable("cscript"); // + " //nologo " + vbsExe;
    QString execString = QString("cscript");
    //args << QCoreApplication::applicationDirPath() + "/python/runjupyter.vbs";
    //m_jupyterLabServer_process.setArguments(args);
    //m_jupyterLabServer_process.setWorkingDirectory(QCoreApplication::applicationDirPath() + "/python");
    //m_jupyterLabServer_process.setProcessChannelMode(QProcess::MergedChannels);
    m_jupyterLabServer_process.start(execString, args);
    auto success = m_jupyterLabServer_process.waitForStarted(60000);
    if (!success) {
        qDebug() << "Error starting Jupyter Lab: " << m_jupyterLabServer_process.errorString();
    }
    else {
        qInfo() << "Started Jupyter Lab ";
        qInfo() << "Open http://localhost:8888/lab in the browser - if it has not already opened";
    }
    return success;
}


void XeusKernel::onReadyStandardOutput()
{
    m_jupyterLabServer_process.setReadChannel(QProcess::StandardOutput);
    while (m_jupyterLabServer_process.canReadLine()) {
        qInfo() << QString::fromLocal8Bit(m_jupyterLabServer_process.readLine());
    }
}

void XeusKernel::onReadyStandardError()
{
    m_jupyterLabServer_process.setReadChannel(QProcess::StandardError);
    while (m_jupyterLabServer_process.canReadLine()) {
        qWarning() << QString::fromLocal8Bit(m_jupyterLabServer_process.readLine());
    }
}
