#pragma once

// QTs slots clashes with Python.h slots
#undef slots
#include <xeus-python/xinterpreter.hpp>
#define slots Q_SLOTS

#include <memory>

#include <QString>

namespace pybind11 {
    class scoped_interpreter;
}

/**
 * This class wraps the xeus python interpreter 
 * and initializes the pybind11 data module
 * 
 * Similar uses: 
 * xSlicerInterpreter.cxx in SlicerJupyter
 * https://github.com/Slicer/SlicerJupyter/blob/724809ab27667793a0438af6e087ff7decd7d1fe/JupyterKernel/xSlicerInterpreter.h
 */

class XeusInterpreter : public xpyt::interpreter
{
public:
    XeusInterpreter(const QString& pluginVersion = "");
    virtual ~XeusInterpreter() = default;

private:
    void configure_impl() override;

    void execute_request_impl(send_reply_callback cb,
                              int execution_counter,
                              const std::string& code,
                              xeus::execute_request_config config,
                              nl::json user_expressions) override;

    nl::json kernel_info_request_impl() override;

private:
    std::unique_ptr<pybind11::scoped_interpreter>   _init_guard = {};
    QString                                         _pluginVersion = "";
};
