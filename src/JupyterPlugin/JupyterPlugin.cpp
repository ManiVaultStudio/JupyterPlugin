#include "JupyterPlugin.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "XeusKernel.h"

#undef slots
#include <pybind11/embed.h>
#include <Python.h>
#include <pybind11/pybind11.h>
#define slots Q_SLOTS

Q_PLUGIN_METADATA(IID "studio.manivault.JupyterPlugin")

using namespace mv;
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

JupyterPlugin::JupyterPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _pKernel(std::make_unique<XeusKernel>()),
    _connectionFilePath(this, "Connection file", QDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0]).filePath("connection.json"))
{
}

JupyterPlugin::~JupyterPlugin()
{
    _pKernel->stopKernel();
}

// https://pybind11.readthedocs.io/en/stable/reference.html#_CPPv422initialize_interpreterbiPPCKcb
void JupyterPlugin::init()
{  
    {
        // PyConfig config;
        // PyConfig_InitPythonConfig(&config);
        // config.parse_argv = 0;

        // //PyConfig_InitIsolatedConfig(&config);

        // QString pythonHome = QString::fromLocal8Bit(qgetenv("PYTHONHOME"));
        // PyConfig_SetString(&config, &config.home, pythonHome.toStdWString().c_str());

        // QString program_name = pythonHome + "bin/python3";
        // PyConfig_SetString(&config, &config.program_name, program_name.toStdWString().c_str());

        // // Set Python home to your Conda environment
        // // wchar_t* home = Py_DecodeLocale("/home/alxvth/miniconda3/envs/mv_test_13", NULL);
        // // if (home == NULL) 
        // //     qDebug() << "FAIL";
        
        // // PyConfig_SetString(&config, &config.home, home);

        // config.install_signal_handlers = 1;

        // qDebug() << "Init";

        // py::initialize_interpreter(&config, 0, nullptr, false);

        py::scoped_interpreter guard{};
        qDebug() << "After scoped_interpreter";
        print_pldd_n();

        // qDebug() << "PATH: " << (getenv("PATH") ? getenv("PATH") : "<not set>");
        // qDebug() << "PYTHONHOME: " << (getenv("PYTHONHOME") ? getenv("PYTHONHOME") : "<not set>");
        // qDebug() << "PYTHONPATH: " << (getenv("PYTHONPATH") ? getenv("PYTHONPATH") : "<not set>");

        //py::scoped_interpreter guard{};

        qDebug() << "import sys...";

        py::exec(R"(
            import sys
            print(sys.path)
            print(sys.prefix)
        )");

        qDebug() << "Import socket...";

        py::exec(R"(
            import socket
        )");

        //py::finalize_interpreter();
    }

    qDebug() << "Switch to Xeus...";

    QString jupyter_configFilepath = _connectionFilePath.getFilePath();
    QString pluginVersion = QString::fromStdString(getVersion().getVersionString());

    _init_guard = std::make_unique<pybind11::scoped_interpreter>(); // start the interpreter and keep it alive

    _pKernel->startKernel(jupyter_configFilepath, pluginVersion);
}

// =============================================================================
// Plugin Factory
// =============================================================================

ViewPlugin* JupyterPluginFactory::produce()
{
    return new JupyterPlugin(this);
}

mv::DataTypes JupyterPluginFactory::supportedDataTypes() const
{
    return { };
}
