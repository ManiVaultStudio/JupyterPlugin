#include "XeusInterpreter.h"

#include "MVData.h"
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

namespace py = pybind11;

#include <iostream>
#include <fstream>
#include <string>
#include <deque>
#include <cstdio>
#include <cstdlib>
#include <unistd.h> 

static void print_pldd() {
    // Get our own PID
    pid_t pid = getpid();

    // Construct the command string
    std::string command = "pldd " + std::to_string(pid);

    // Open a pipe to read the output of pldd
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to run pldd\n";
        return;
    }

    // Read and print the output line by line
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::cout << buffer;
    }

    std::cout << std::endl;

    // Close the pipe
    pclose(pipe);
}

static void print_pldd_n(size_t numLines = 5) {
    // Get our own PID
    pid_t pid = getpid();

    // Construct the command string
    std::string command = "pldd " + std::to_string(pid);

    // Open a pipe to read the output of pldd
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to run pldd\n";
        return;
    }

    // Create a deque to store the last 5 lines
    std::deque<std::string> lines;
    char buffer[256];

    // Read the output line by line
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        // Store each line in the deque
        lines.push_back(buffer);

        // If we have more than 5 lines, remove the oldest
        if (lines.size() > numLines) {
            lines.pop_front();
        }
    }

    std::cout << "pldd " << pid << std::endl;

    // Print the last 5 lines
    for (const auto& line : lines) {
        std::cout << line;
    }

    std::cout << std::endl;

    // Close the pipe
    pclose(pipe);
}


XeusInterpreter::XeusInterpreter(const QString& pluginVersion):
    xpyt::interpreter(false, false),
    _pluginVersion(pluginVersion)
{
    m_release_gil_at_startup = false;
}

void XeusInterpreter::configure_impl()
{
    auto handle_comm_opened = [](xeus::xcomm&& comm, const xeus::xmessage&) {
        std::cerr << "Comm opened for target: " << comm.target().name() << std::endl;
    };

    try {
        qDebug() << "xpyt::interpreter::configure_impl";
        print_pldd_n();

        xpyt::interpreter::configure_impl();
        comm_manager().register_comm_target("echo_target", handle_comm_opened);

        qDebug() << "py::gil_scoped_acquire";
        py::gil_scoped_acquire acquire;

        qDebug() << "get_MVData_module";
        py::module MVData_module = get_MVData_module();
        MVData_module.doc() = "Provides access to low level ManiVaultStudio core functions";

        py::module sys = py::module::import("sys");
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
