# Configuration file for jupyter-server.

c = get_config()  #noqa

#------------------------------------------------------------------------------
# MultiKernelManager(LoggingConfigurable) configuration
#------------------------------------------------------------------------------
## A class for managing multiple kernels.

## The name of the default kernel to start
#  Default: 'python3'
c.MultiKernelManager.default_kernel_name = 'ManiVaultStudio'

## The kernel manager class.  This is configurable to allow
#          subclassing of the KernelManager for customized behavior.
#  Default: 'jupyter_client.ioloop.IOLoopKernelManager'
# c.MultiKernelManager.kernel_manager_class = 'jupyter_client.ioloop.IOLoopKernelManager'

## Share a single zmq.Context to talk to all my kernels
#  Default: True
# c.MultiKernelManager.shared_context = True

#------------------------------------------------------------------------------
# MappingKernelManager(MultiKernelManager) configuration
#------------------------------------------------------------------------------
## A KernelManager that handles
#      - File mapping
#      - HTTP error handling
#      - Kernel message filtering

## Whether to send tracebacks to clients on exceptions.
#  Default: True
# c.MappingKernelManager.allow_tracebacks = True

## White list of allowed kernel message types.
#          When the list is empty, all message types are allowed.
#  Default: []
# c.MappingKernelManager.allowed_message_types = []

## Whether messages from kernels whose frontends have disconnected should be
#  buffered in-memory.
#  
#          When True (default), messages are buffered and replayed on reconnect,
#          avoiding lost messages due to interrupted connectivity.
#  
#          Disable if long-running kernels will produce too much output while
#          no frontends are connected.
#  Default: True
# c.MappingKernelManager.buffer_offline_messages = True

## Whether to consider culling kernels which are busy.
#          Only effective if cull_idle_timeout > 0.
#  Default: False
# c.MappingKernelManager.cull_busy = False

## Whether to consider culling kernels which have one or more connections.
#          Only effective if cull_idle_timeout > 0.
#  Default: False
# c.MappingKernelManager.cull_connected = False

## Timeout (in seconds) after which a kernel is considered idle and ready to be culled.
#          Values of 0 or lower disable culling. Very short timeouts may result in kernels being culled
#          for users with poor network connections.
#  Default: 0
# c.MappingKernelManager.cull_idle_timeout = 0

## The interval (in seconds) on which to check for idle kernels exceeding the
#  cull timeout value.
#  Default: 300
# c.MappingKernelManager.cull_interval = 300

## The name of the default kernel to start
#  See also: MultiKernelManager.default_kernel_name
c.MappingKernelManager.default_kernel_name = 'ManiVaultStudio'

## Timeout for giving up on a kernel (in seconds).
#  
#          On starting and restarting kernels, we check whether the
#          kernel is running and responsive by sending kernel_info_requests.
#          This sets the timeout in seconds for how long the kernel can take
#          before being presumed dead.
#          This affects the MappingKernelManager (which handles kernel restarts)
#          and the ZMQChannelsHandler (which handles the startup).
#  Default: 60
# c.MappingKernelManager.kernel_info_timeout = 60

## The kernel manager class.  This is configurable to allow
#  See also: MultiKernelManager.kernel_manager_class
# c.MappingKernelManager.kernel_manager_class = 'jupyter_client.ioloop.IOLoopKernelManager'

#  Default: ''
# c.MappingKernelManager.root_dir = ''

## Share a single zmq.Context to talk to all my kernels
#  See also: MultiKernelManager.shared_context
# c.MappingKernelManager.shared_context = True

## Message to print when allow_tracebacks is False, and an exception occurs
#  Default: 'An exception occurred at runtime, which is not shown due to security reasons.'
# c.MappingKernelManager.traceback_replacement_message = 'An exception occurred at runtime, which is not shown due to security reasons.'

