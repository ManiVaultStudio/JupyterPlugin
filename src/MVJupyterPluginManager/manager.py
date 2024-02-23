from jupyter_server.services.kernels.kernelmanager import MappingKernelManager, MultiKernelManager
from pathlib import Path
import os

import logging
logging.basicConfig(level=logging.DEBUG)
_log = logging.getLogger(__name__)

class ExternalMappingKernelManager(MappingKernelManager):
    """A Kernel manager that connects to a IPython kernel started outside of Jupyter"""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.pinned_superclass = ExternaMultiKernelManager
        self.pinned_superclass.__init__(self, *args, **kwargs)

    def _attach_to_mvjupyter_kernel(self, kernel_id):
        self.log.info(f'Attaching {kernel_id} to an existing kernel...')
        kernel = self.get_kernel(kernel_id)
        port_names = ['shell_port', 'stdin_port', 'iopub_port', 'hb_port', 'control_port']
        port_names = kernel._random_port_names if getattr(kernel, '_random_port_names', None) else port_names
        for port_name in port_names:
            setattr(kernel, port_name, 0)

        #Connect to kernel started by ManiVault JupyterPlugin.
        # retrieve from environment or arg
        #connection_file = Path('D:/TempProj/DevBundle/Jupyter/install/Debug/external_kernels/ManiVault/connection.json')
        connection_file = Path(os.environ["MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE"])

        if not connection_file.exists():
            _log.warning(f"Jupyter connection file '{connection_file}' does not exist.")

        _log.info(f'ManiVault Jupyter kernel = {connection_file}')
        kernel.load_connection_file(connection_file)

    async def start_kernel(self, **kwargs):
        """Attach to the kernel started by ManiVault JupyterPlugin.
        """
        kernel_id = await super(ExternalMappingKernelManager, self).start_kernel(**kwargs)
        self._attach_to_mvjupyter_kernel(kernel_id)
        return kernel_id


class ExternaMultiKernelManager(MultiKernelManager):
    """Subclass of MultiKernelManager to prevent restarting of the ManiVault JupyterPlugin kernel"""    

    def restart_kernel(self, *args, **kwargs):
        raise NotImplementedError("Restarting a kernel running in ManiVault JupyterPlugin is not supported.")
    
    async def _async_restart_kernel(self, *args, **kwargs):
        raise NotImplementedError("Restarting a kernel running in ManiVault JupyterPlugin is not supported.")

    def shutdown_kernel(self, *args, **kwargs):
        raise NotImplementedError("Shutting down a kernel running in ManiVault JupyterPlugin is not supported.")

    async def _async_shutdown_kernel(self, *args, **kwargs):
        raise NotImplementedError("Shutting down a kernel running in ManiVault JupyterPlugin is not supported.")

    def shutdown_all(self, *args, **kwargs):
        raise NotImplementedError("Shutting down a kernel running in ManiVault JupyterPlugin is not supported.")

    async def _async_shutdown_all(self, *args, **kwargs):
        raise NotImplementedError("Shutting down a kernel running in ManiVault JupyterPlugin is not supported.")