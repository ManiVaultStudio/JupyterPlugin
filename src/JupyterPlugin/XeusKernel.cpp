#include "XeusKernel.h"

#include "XeusInterpreter.h"
#include "XeusServer.h"

#include <QDebug>

#include <nlohmann/json_fwd.hpp>

#include <xeus/xeus_context.hpp>
#include <xeus/xhistory_manager.hpp>
#include <xeus/xkernel.hpp>
#include <zmq.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

void XeusKernel::startKernel(const std::string& connection_path, const std::string& pluginVersion, const std::string& workingDir)
{
    using context_type = xeus::xcontext_impl<zmq::context_t>;

    auto interpreter = std::make_unique<XeusInterpreter>(pluginVersion);
    xpyt::interpreter* interpreter_ptr = interpreter.get();

    try {
        m_kernel = std::make_unique<xeus::xkernel>(
            /*config: not used here */
            /*user_name      */ xeus::get_user_name(),
            /*context        */ std::make_unique<context_type>(),
            /*interpreter    */ std::move(interpreter),
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

    qInfo() << "Xeus Kernel settings:";
    qInfo().noquote() << QString::fromStdString(kernel_spec.dump(4));

    qDebug() << "Writing connection config to " << connection_path;
    std::ofstream configFile(connection_path);
    configFile << kernel_spec;

    m_kernel->start();

    if (!workingDir.empty() && std::filesystem::exists(workingDir))
    {
        qDebug() << "Setting notebook working directory to " << workingDir;
        interpreter_ptr->execute_request(
            xeus::xrequest_context{},       // Minimal context - no header or message ID needed for internal startup calls
            [](nl::json reply) {},  // No-op callback - we don't need the reply during initialization
            std::format("import os; os.chdir('{}')", workingDir),
            xeus::execute_request_config{ false, false, false },
            nl::json::object()              // empty user_expressions
        );
    }
    
}

void XeusKernel::stopKernel()
{
    m_kernel->stop();
}
