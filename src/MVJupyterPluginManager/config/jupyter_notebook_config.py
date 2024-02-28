import MVJupyterPluginManager
c.NotebookApp.allow_remote_access = False
c.NotebookApp.open_browser = True
c.MappingKernelManager.default_kernel_name = 'ManiVaultStudio'
c.KernelSpecManager.ensure_native_kernel = False
c.ServerApp.kernel_manager_class = MVJupyterPluginManager.ExternalMappingKernelManager
c.KernelProvisionerFactory.default_provisioner_name = 'mvjupyterplugin-existing-provisioner'