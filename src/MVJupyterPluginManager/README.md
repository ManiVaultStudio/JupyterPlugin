## MVJupyterPluginManager

A kernel manager that allows JupyterLab to connect to an externally started kernel. In out case this is the kernel that runs in the JupyterPlugin.

The code is largely copied from the examples in https://github.com/pyxll/pyxll-jupyter/tree/master/pyxll_jupyter/kernel_managers, https://github.com/SciQLop/SciQLop/tree/main/SciQLop/Jupyter and https://github.com/ebanner/pynt/tree/master/codebook

JupyterLab can be started with the [ServerApp.kernel_manager_class](https://jupyterlab-server.readthedocs.io/en/stable/api/app-config.html) option. In this case this is 

```
jupyterlab --ServerApp.kernel_manager_class=MVJupyterPluginManager.ExternalMappingKernelManager
```

Baldur van Lew
2024-02-06

Note: In principle Jupyverse show be capable of connecting to an external kernel but this does not seem to work https://github.com/jupyter-server/jupyverse/issues/385