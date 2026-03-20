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

void XeusKernel::startKernel(const QString& connection_path, const QString& pluginVersion)
{
    using context_type = xeus::xcontext_impl<zmq::context_t>;

    try {
        m_kernel = std::make_unique<xeus::xkernel>(
            /*config: not used here */
            /*user_name      */ xeus::get_user_name(),
            /*context        */ std::make_unique<context_type>(),
            /*interpreter    */ std::make_unique<XeusInterpreter>(pluginVersion),
            /*server_builder */ make_XeusServer,
            /*history_manager*/ xeus::make_in_memory_history_manager(),
            /*logger         */ nullptr
        );
    } catch (const std::exception& ex) {
        qDebug() << "Cannot create xeus kernel: " << ex.what();
        m_kernel.reset();
        return;
    }

    // Save the config that was generated
    const auto& kernel_config           = m_kernel->get_config();
    nlohmann::json kernel_spec          = {};
    kernel_spec["transport"]            = kernel_config.m_transport.c_str();
    kernel_spec["ip"]                   = kernel_config.m_ip.c_str();
    kernel_spec["control_port"]         = std::stoi(kernel_config.m_control_port);
    kernel_spec["shell_port"]           = std::stoi(kernel_config.m_shell_port);
    kernel_spec["stdin_port"]           = std::stoi(kernel_config.m_stdin_port);
    kernel_spec["iopub_port"]           = std::stoi(kernel_config.m_iopub_port);
    kernel_spec["hb_port"]              = std::stoi(kernel_config.m_hb_port);
    kernel_spec["signature_scheme"]     = kernel_config.m_signature_scheme.c_str();
    kernel_spec["key"]                  = kernel_config.m_key.c_str();
    kernel_spec["language"]             = "python";
    kernel_spec["kernel_name"]          = "ManiVaultStudio";
    kernel_spec["env"]["PYTHONSTARTUP"] = "D:/avieth/Documents/ManiVault/DevBundle/allmain/install/RelWithDebInfo/examples/JupyterPlugin/projects/volume_data";

    qInfo() << "Xeus Kernel settings:";
    qInfo().noquote() << QString::fromStdString(kernel_spec.dump(4));

    qDebug() << "Writing connection config to " << connection_path;
    std::ofstream configFile(connection_path.toUtf8());
    configFile << kernel_spec;

    m_kernel->start();
}

void XeusKernel::stopKernel()
{
    m_kernel->stop();
}
