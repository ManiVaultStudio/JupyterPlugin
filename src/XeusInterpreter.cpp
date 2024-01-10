#include "XeusInterpreter.h"

XeusInterpreter::XeusInterpreter()
{
   
}

void XeusInterpreter::configure_impl()
{
    xpyt::interpreter::configure_impl();
}

// Execute incoming code and publish the result
nl::json XeusInterpreter::execute_request_impl(int execution_counter,
  const std::string& code,
  bool store_history,
  bool silent,
  nl::json user_expressions,
  bool allow_stdin) 
{
    return xpyt::interpreter::execute_request_impl(
        execution_counter,
        code,
        store_history,
        silent,
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
    return xpyt::interpreter::kernel_info_request_impl();
}

void XeusInterpreter::shutdown_request_impl()
{
    return xpyt::interpreter::shutdown_request_impl();
}




