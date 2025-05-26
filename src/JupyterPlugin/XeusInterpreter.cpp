#include "XeusInterpreter.h"

#include "JupyterPlugin.h"
#include "PythonBuildVersion.h"

#include <iostream>
#include <stdexcept>
#include <string>

#include <xeus/xhelper.hpp>
#include <xeus/xcomm.hpp>

#undef slots
#include <pybind11/embed.h>
#define slots Q_SLOTS

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QOperatingSystemVersion>

namespace py = pybind11;

XeusInterpreter::XeusInterpreter(const QString& pluginVersion):
    xpyt::interpreter(false, false),
    _pluginVersion(pluginVersion)
{
    m_release_gil_at_startup = false;
}

void XeusInterpreter::configure_impl()
{
    this->redirect_output();
    auto handle_comm_opened = [](xeus::xcomm&& comm, const xeus::xmessage&) {
        std::cerr << "Comm opened for target: " << comm.target().name() << std::endl;
    };

    try {
        xpyt::interpreter::configure_impl();
        comm_manager().register_comm_target("echo_target", handle_comm_opened);

        py::gil_scoped_acquire acquire;

        py::module sys = py::module::import("sys");
        py::dict modules = sys.attr("modules");

        if (!modules.contains("mvstudio_core")) {
            JupyterPlugin::init_mv_communication_module();
            py::module MVData_module = *(JupyterPlugin::mv_communication_module.get());

            sys.attr("modules")["mvstudio_core"] = MVData_module;
        }
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "MVData modules failed to load: " << e.what() << std::endl;
        throw std::runtime_error("Required python modules not available");
    }
    catch (...) 
    {
        std::cerr << "Unable to start jupyter notebook..." << std::endl;

        if(QOperatingSystemVersion::currentType() != QOperatingSystemVersion::Windows && qEnvironmentVariableIsSet("CONDA_PREFIX"))
        {
            const QString condaPrefix = QString::fromLocal8Bit(qgetenv("CONDA_PREFIX"));
            QFileInfo condaPrefixPath(condaPrefix);
            std::string environmentName = QDir(condaPrefixPath.absoluteFilePath()).dirName().toStdString();
            std::string pyVersion = std::to_string(pythonVersionMajor) + "." + std::to_string(pythonVersionMinor);

            std::cerr << "If you are using a conda environment, setting LD_PRELOAD before restarting the application might help:\n";
            std::cerr << "   conda activate " + environmentName + "\n";
            std::cerr << "   CURRENT_PYTHON_PATH=$(find ${CONDA_PREFIX} -name libpython" + pyVersion + "* 2>/dev/null | head -n 1)\n";
            std::cerr << "   conda env config vars set LD_PRELOAD=$CURRENT_PYTHON_PATH --name " + environmentName + "\n";
            std::cerr << "   conda deactivate && conda activate " + environmentName + "\n";
            std::cerr << "\n";
            std::cerr << "If you are using a mamba environment, setting LD_PRELOAD has to be done slightly differently:\n";
            std::cerr << "   CURRENT_PYTHON_PATH=$(find ${CONDA_PREFIX} -name libpython" + pyVersion + "* 2>/dev/null | head -n 1)\n";
            std::cerr << "   echo -e \"{\\\"env_vars\\\": {\\\"LD_PRELOAD\\\": \\\"${CURRENT_PYTHON_PATH}\\\"}}\" >> ${CONDA_PREFIX}/conda-meta/state\n";
            std::cerr << "   micromamba deactivate && micromamba activate " + environmentName + "\n";
            std::cerr << std::endl;
        }
        throw std::runtime_error("Cannot start notebook");
    }

}

void XeusInterpreter::execute_request_impl(send_reply_callback cb,
    int execution_counter,
    const std::string& code,
    xeus::execute_request_config config,
    nl::json user_expressions)
{
    qDebug() << "Python: " << code.c_str();

    xpyt::interpreter::execute_request_impl(cb,
        execution_counter,
        code,
        config,
        user_expressions);
}

nl::json XeusInterpreter::kernel_info_request_impl()
{
    std::string banner = R"(
          __  __             ___     __          _ _   
         |  \/  | __ _ _ __ (_) \   / /_ _ _   _| | |_ 
         | |\/| |/ _` | '_ \| |\ \ / / _` | | | | | _|
         | |  | | (_| | | | | | \ V / (_| | |_| | | |_ 
         |_|  |_|\__,_|_| |_|_|  \_/_\__,_|\__,_|_|\__|
               / ___|| |_ _   _  __| (_) ___           
               \___ \| __| | | |/ _` | |/ _ \          
                ___) | |_| |_| | (_| | | (_) |         
               |____/ \__|\__,_|\__,_|_|\___/          
                                                       
        
                 ManiVaultStudio JupyterPlugin
          A python kernel with access to the ManiVault Studio)";

    return xeus::create_info_reply(xeus::get_protocol_version(),
        "ManiVault JupyterPlugin",
        _pluginVersion.toStdString(), // as defined in JupyterPlugin.json
        "python",
        std::to_string(pythonVersionMajor) + "." + std::to_string(pythonVersionMinor),
        "text/x-python",
        "py",
        "",
        "",
        "",
        banner);

    //return xpyt::interpreter::kernel_info_request_impl();
}

PYBIND11_MODULE(mvtest, m) {
    m.doc() = "ManiVault test module";

    // Add bindings here
    m.def("sayhello", []() {
        return "Hello, World!";
    });
}
