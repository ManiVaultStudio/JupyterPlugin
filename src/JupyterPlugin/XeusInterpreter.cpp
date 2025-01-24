#include "XeusInterpreter.h"

#include "MVData.h"

#include <iostream>
#include <stdexcept>
#include <string>

#include <xeus/xcomm.hpp>
#include <xeus/xhelper.hpp>
#include <xeus/xinterpreter.hpp>
#include <xeus-python/xinterpreter.hpp>

#include <pybind11/gil.h>
#include <pybind11/pybind11.h>

#include <QDebug>

namespace py = pybind11;

XeusInterpreter::XeusInterpreter(const QString& pluginVersion):
    xpyt::interpreter(),
    _pluginVersion(pluginVersion)
{
}

void XeusInterpreter::configure_impl()
{
    auto handle_comm_opened = [](xeus::xcomm&& comm, const xeus::xmessage&) {
        std::cerr << "Comm opened for target: " << comm.target().name() << std::endl;
    };

    try {
        _init_guard = std::make_unique<pybind11::scoped_interpreter>();

        xpyt::interpreter::configure_impl();
        comm_manager().register_comm_target("echo_target", handle_comm_opened);

        py::gil_scoped_acquire acquire;
        py::module sys = py::module::import("sys");

        py::module MVData_module = get_MVData_module();
        MVData_module.doc() = "Provides access to low level ManiVaultStudio core functions";
        sys.attr("modules")["mvstudio_core"] = MVData_module;
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "MVData modules failed to load: " << e.what() << std::endl;
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
        _pluginVersion.toStdString(),
        "python",
        "3.11",
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
