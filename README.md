# Python integration into ManiVault

This project provides [Python](https://en.wikipedia.org/wiki/Python_(programming_language)) integration into ManiVault. It allows to read and write from and to the ManiVault data hierarchy. 

[JupyterLab](https://jupyter.org/) is used to interact with ManiVault via Python. Upon starting the Python integration, a notebook is automatically opened.


## Building

0. Clone this repository
```bash
git clone git@github.com:ManiVaultStudio/JupyterPlugin.git
```
1. Setup a python 3.11 environment, 
- with [venv](https://docs.python.org/3.11/library/venv.html):
```bash
cd YOUR_LOCAL_PATH\AppData\Local\Programs\Python\Python311
.\python.exe -m venv ..\ManiVaultPythonPluginBuild
```
- with [conda](https://docs.conda.io/projects/conda/en/latest/user-guide/tasks/manage-environments.html)
```bash
conda create -n ManiVaultPythonPluginBuild python=3.11
```
2. Pass `Python_EXECUTABLE` and `ManiVaultDir` to cmake:
    - Define `ManiVault_DIR` pointing to the ManiVault cmake file folder, e.g. `YOUR_LOCAL_PATH/install/cmake/mv`
    - Define `Python_EXECUTABLE` pointing to the python exe, e.g. 
        - venv: `YOUR_LOCAL_PATH/AppData/Local/Programs/Python/ManiVaultPythonPluginBuild/Scripts/python.exe`
        - Conda: `YOUR_LOCAL_PATH/AppData/Local/miniconda3/envs/ManiVaultPythonPluginBuild/python.exe`

3. (On Windows) Use [vcpkg](https://github.com/microsoft/vcpkg) and define `DCMAKE_TOOLCHAIN_FILE="[YOURPATHTO]/vcpkg/scripts/buildsystems/vcpkg.cmake"` to install the OpenSSL dependency

This projects builds two ManiVault plugins, a communication bridge `JupyterPlugin311` between the ManiVault application and Python as well as a `JupyterLauncher` plugin, which is used to launch a Python kernel and Jupyter notebook.

## Dependencies

Linux:
```
sudo apt install libtbb-dev libsodium-dev libssl-dev uuid-dev
```

Mac:
```
brew --prefix libomp
```

## Usage

You can use any local python environment (currently restricted to 3.11) with this plugin to interact with ManiVault. 
You need to provide a path to a python interpreter in the ManiVault settings for this. Go to `File` -> `Settings` -> `Plugin: Jupyter Launcher` -> `Python interpreter` and navigate to your local python interpreter. (These can be the same as used for biulding or any other).

Upon the first start with a given interpreter, two comminication modules are automatically installed, which will help translate between ManiVault's data hierarchy and your Python script.

To access data from ManiVault:
```python
import mvstudio.data
h = mvstudio.data.Hierarchy()
print(h)
```


## Use of Jupyter logo

This plugin kernel based on the Xeus kernel is compatible with Jupyter software. As such we use the unmodified Jupyter logo to make this clear to end users. This usage follows the rules set out in the [Jupyter documentation](https://jupyter.org/governance/trademarks.html#uses-that-never-require-approval).

## Kernel architecture

The kernel relies on Jupyter-Xeus components to expose a python 3.11 environment. 

The architecture of the kernel is based on [Slicer/SlicerJupyter](https://github.com/Slicer/SlicerJupyter). The correspondence is shown in the table below.

ManiVault JupyterPlugin | Slicer SlicerJupyter extension
---| --- 
JupyterXeusKernel | qSlicerJupyterKernelModule 
JupyterPlugin | qSlicerJupyterKernelModuleWidget


