#pragma once

// QTs slots clashes with Python.h slots
#undef slots
#include <xeus-python/xinterpreter.hpp>
#define slots Q_SLOTS

#include <QString>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

/**
 * This class wraps the xeus python interpreter
 * In the slicer implementation (xSlicerInterpreter.cxx in SlicerJupyter)
 * the wrapping adds additional functionality.ine
 * In the first iteration this wrapper is effectively a noop 
 * but it provides a point for additional functionality when 
 * processing python code.
*/

class XeusInterpreter : public xpyt::interpreter
{
public:
    XeusInterpreter(const QString& pluginVersion = "");
    virtual ~XeusInterpreter() = default;

private:
    void configure_impl() override;

    nl::json execute_request_impl(int execution_counter,
                                  const std::string& code,
                                  bool silent,
                                  bool store_history,
                                  nl::json user_expressions,
                                  bool allow_stdin) override;

    nl::json complete_request_impl(const std::string& code,
                                int cursor_pos) override;

    nl::json inspect_request_impl(const std::string& code,
                               int cursor_pos,
                               int detail_level) override;

    nl::json is_complete_request_impl(const std::string& code) override;

    nl::json kernel_info_request_impl() override;

    void shutdown_request_impl() override;

private:
    pybind11::scoped_interpreter* _python_interpreter = nullptr;
    QString _pluginVersion = "";
};
