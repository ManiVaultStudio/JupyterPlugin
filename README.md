# Python integration into ManiVault using JupyterNotebooks

This project provided python 3.11 integration into ManiVault. In the current release this is limited to reading and writing from and to the ManiVault DataHierarchy. As yet there is not interface to other ManiVault subcomponents or plugins.

Using the JupyterPlugin data can loaded into the DataHierarchy for which there is no loader plugin available. Data from the hierarchy can be subjected to additional processing, analysis or viewing supported by standard python packages.

## Building

- Setup a python environment, e.g.
```bash
conda create -n ManiVaultPythonPlugin python=3.11
conda activate ManiVaultPythonPlugin
pip install poetry
```
- Define `ManiVault_DIR` pointing to the ManiVault cmake file folder, e.g. `YOUR_LOCAL_PATH/install/cmake/mv`
- Define `Python_EXECUTABLE` pointing to the python exe, e.g. `YOUR_LOCAL_PATH/AppData/Local/miniconda3/envs/ManiVaultPythonPlugin/python.exe`


## Python environment requirements.

The following python and library versions must be installed.


## JupyterLauncher -> JupyterPlugin (kernel host)

The JupyterLauncher plugin launches the JupyterPlugin that actually provides a ManiVaultStudio python kernel for any Jupyter Lab server. To provide python functionality there needs to be a python environment available. 

The plugin hosted python kernel supplies a module, MVData, containing methods that can be accessed from a Jupyter notebook. 

Although the articture is a view plugin the actual view should be access via a web browser. The plugin it's shows no more than c aconfiguration widget. In addition the URL (along with token) to be used to access this kernel through

## Use of Jupyter logo

This plugin kernel based on the Xeus kernel is compatible with Jupyter software. As such we use the unmodified Jupyter logo to make this clear to end users. This usage follows the rules set out in the [Jupyter documentation](https://jupyter.org/governance/trademarks.html#uses-that-never-require-approval).

## Building

1. Setup a python 3.11 environment and install `poetry`
2. Pass `Python_EXECUTABLE` and `ManiVaultDir` to cmake
3. (On Windows) Use [vcpkg](https://github.com/microsoft/vcpkg) and define `DCMAKE_TOOLCHAIN_FILE="[YOURPATHTO]/vcpkg/scripts/buildsystems/vcpkg.cmake"`

For example:
```
conda create -n ManiVaultPythonPlugin python=3.11
conda activate ManiVaultPythonPlugin
pip install poetry

Python_EXECUTABLE="[YOUR_PATH_TO]/miniconda3/envs/ManiVaultPythonPlugin/python.exe"
ManiVault_DIR="[YOUR_PATH_TO]/install/cmake/mv"

install openssl with vcpkg
CMAKE_TOOLCHAIN_FILE="[YOUR_PATH_TO]/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

The Build produces a JupyterLauncher and a JupyterPlugin. The Jupyter-Xeus dependencies and their dependencies (libzmq and a number of header-only libraries) are also built and linked statically to simplify installation.

The JupyterLauncher is a regular plugin installed in the Plugins directory but the JupyterPlugin 

In the JupyterLauncher select the following a directory containing The JupyterLauncher allows the user to select a python

### Kernel architecture

The kernel relies on Jupyter-Xeus components to expose a python 3.11 environment. 

The architecture of the kernel is based on Slicer/SlicerJupyter.  The correspondance is shown in the table below.

ManiVault JupyterPlugin | Slicer SlicerJupyter extension | Manivault Example
---| --- | ---
JupyterXeusKernel | qSlicerJupyterKernelModule | n.a.
JupyterPlugin | qSlicerJupyterKernelModuleWidget | ExampleViewPlugin


