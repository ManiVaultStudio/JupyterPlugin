#pragma once

// QTs slots clashes with Python.h slots
#undef slots
#include <xeus-python/xinterpreter.hpp>
#define slots Q_SLOTS

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

/**
 * This class wraps the xeus python interpreter
 * In the slicer implementation (xSlicerInterpreter.cxx in SlicerJupyter)
 * the wrapping adds additional functionality.ine
 * In the first iteration this wapper is effectively a noop 
 * but it provides a point for additional functionality when 
 * processing python code.
*/

class XeusInterpreter : public xpyt::interpreter
{
public:
    XeusInterpreter();
    virtual ~XeusInterpreter() = default;

private:
    void configure_impl() override;

    void execute_request_impl(send_reply_callback cb,
                              int execution_counter,
                              const std::string& code,
                              xeus::execute_request_config config,
                              nl::json user_expressions) override;

    nl::json complete_request_impl(const std::string& code,
                                int cursor_pos) override;

    nl::json inspect_request_impl(const std::string& code,
                               int cursor_pos,
                               int detail_level) override;

    nl::json is_complete_request_impl(const std::string& code) override;

    nl::json kernel_info_request_impl() override;

    void shutdown_request_impl() override;

    pybind11::scoped_interpreter* python_interpreter;
    py::module comm_module;
};