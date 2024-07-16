#include "XeusInterpreter.h"
#include "MVData.h"
#include <QDebug>
#include <pybind11/pybind11.h>
#include <xeus/xhelper.hpp>
//#include <xcomm.hpp>
#include <iostream>
namespace py = pybind11;

XeusInterpreter::XeusInterpreter()
{
    python_interpreter = new pybind11::scoped_interpreter;
}

void XeusInterpreter::configure_impl()
{
    xpyt::interpreter::configure_impl();
    auto handle_comm_opened = [](xeus::xcomm&& comm, const xeus::xmessage&) {
        std::cerr << "Comm opened for target: " << comm.target().name() << std::endl;
    };
    comm_manager().register_comm_target("echo_target", handle_comm_opened);
    //	using function_type = std::function<void(xeus::xcomm&&, const xeus::xmessage&)>;
#ifdef DEBUG
    std::cerr << "JupyterPlugin configured" << std::endl;
#endif

    py::gil_scoped_acquire acquire;
    py::module sys = py::module::import("sys");
    py::module MVData_module = get_MVData_module();
    MVData_module.doc() = "Provides access to low level ManiVaultStudio core functions";
    sys.attr("modules")["mvstudio_core"] = MVData_module;
}

// Execute incoming code and publish the result
nl::json XeusInterpreter::execute_request_impl(int execution_counter,
  const std::string& code,
  bool silent,
  bool store_history,
  nl::json user_expressions,
  bool allow_stdin) 
{
    qDebug() << "code: " << code.c_str();

    return xpyt::interpreter::execute_request_impl(
        execution_counter,
        code,
        silent,
        store_history,
        user_expressions,
        allow_stdin
    );
}

nl::json XeusInterpreter::complete_request_impl(const std::string& code,
                                int cursor_pos)
{
    return xpyt::interpreter::complete_request_impl(
        code,
        cursor_pos);
}

nl::json XeusInterpreter::inspect_request_impl(const std::string& code,
                               int cursor_pos,
                               int detail_level)
{
    return xpyt::interpreter::inspect_request_impl(
        code,
        cursor_pos,
        detail_level
    );
}

nl::json XeusInterpreter::is_complete_request_impl(const std::string& code)
{
    return xpyt::interpreter::is_complete_request_impl(code);
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
        JUPYTERPLUGIN_VERSION,
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

void XeusInterpreter::shutdown_request_impl()
{
    return xpyt::interpreter::shutdown_request_impl();
}

PYBIND11_MODULE(mvtest, m) {
    m.doc() = "ManiVault test module";

    // Add bindings here
    m.def("sayhello", []() {
        return "Hello, World!";
    });
}



