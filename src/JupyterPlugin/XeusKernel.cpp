#include "XeusKernel.h"

#include "XeusInterpreter.h"
#include "XeusServer.h"

#include <QDebug>

#include <nlohmann/json_fwd.hpp>

#include <xeus/xeus_context.hpp>
#include <xeus/xhistory_manager.hpp>
#include <xeus/xkernel.hpp>
#include <zmq.hpp>

#include <fstream>
#include <memory>
#include <string>

XeusKernel::XeusKernel()
{

}

void XeusKernel::startKernel(const QString& connection_path, const QString& pluginVersion)
{
    //// Test without configuration file (default zmq ports)
    //xeus::xconfiguration config = xeus::load_configuration("D:\\TempProj\\DevBundle\\Jupyter\\install\\Debug\\external_kernels\\ManiVault\\connection.json");
    //auto context =  xeus::make_context<zmq::context_t>();
    using context_type = xeus::xcontext_impl<zmq::context_t>;
    using context_ptr = std::unique_ptr<context_type>;
    context_ptr context = context_ptr(new context_type());
    std::unique_ptr<XeusInterpreter> interpreter = std::make_unique<XeusInterpreter>(pluginVersion);
    std::unique_ptr<xeus::xhistory_manager> hist = xeus::make_in_memory_history_manager();
    m_kernel = new xeus::xkernel(
        /*config: noy used here */
        /*user_name*/ xeus::get_user_name(),
        /*context*/ std::move(context),
        /*interpreter*/ std::move(interpreter),
        /*server_builder*/ make_XeusServer,
        /*history_manager*/ std::move(hist),
        /*logger*/ nullptr
    );

    // Save the config that was generated
    const auto& set_config = m_kernel->get_config();
    nlohmann::json jsonObj;
    jsonObj["transport"] = set_config.m_transport.c_str();
    jsonObj["ip"] = set_config.m_ip.c_str();
    jsonObj["control_port"] = std::stoi(set_config.m_control_port);
    jsonObj["shell_port"] = std::stoi(set_config.m_shell_port);
    jsonObj["stdin_port"] = std::stoi(set_config.m_stdin_port);
    jsonObj["iopub_port"] = std::stoi(set_config.m_iopub_port);
    jsonObj["hb_port"] = std::stoi(set_config.m_hb_port);
    jsonObj["signature_scheme"] = set_config.m_signature_scheme.c_str();
    jsonObj["key"] = set_config.m_key.c_str();
    jsonObj["kernel_name"] = "ManiVaultStudio";

    qDebug() << "Writing connection config to " << connection_path;
    std::ofstream configFile(connection_path.toUtf8());
    configFile << jsonObj;

    //const auto& config = m_kernel->get_config();
    qInfo() << "Xeus Kernel settings";
    qInfo() << "transport protocol: " << set_config.m_transport.c_str();
    qInfo() << "IP address: " << set_config.m_ip.c_str();
    qInfo() << "control port: " << set_config.m_control_port.c_str();
    qInfo() << "shell port: " << set_config.m_shell_port.c_str();
    qInfo() << "stdin port: " << set_config.m_stdin_port.c_str();
    qInfo() << "iopub port: " << set_config.m_iopub_port.c_str();
    qInfo() << "hb port: " << set_config.m_hb_port.c_str();
    qInfo() << "signature scheme: " << set_config.m_signature_scheme.c_str();
    qInfo() << "connection key: " << set_config.m_key.c_str();

    m_kernel->start();
}

void XeusKernel::stopKernel()
{

}

