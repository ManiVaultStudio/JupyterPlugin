# Python integration into ManiVault

This project provides [Python](https://en.wikipedia.org/wiki/Python_(programming_language)) integration into [ManiVault](https://github.com/ManiVaultStudio/core). It implements two modes of operation: 
1. a connection to [Jupyter notebooks](https://jupyter.org/), and
2. the ability to run [individual Python scripts](./docs/Python_scripts_for_ManiVault.md). 
An additional Python module is provided which  allows users to push data from Python to ManiVault and retrieve data from ManiVault in Python. 

```bash
git clone git@github.com:ManiVaultStudio/JupyterPlugin.git
```

## Building

1. Setup a python environment (preferably 3.11+):
- with [venv](https://docs.python.org/3.11/library/venv.html):
```bash
cd YOUR_LOCAL_PATH\AppData\Local\Programs\Python\Python311
.\python.exe -m venv ..\ManiVaultPythonPluginBuild
```
- with [conda](https://docs.conda.io/projects/conda/en/latest/user-guide/tasks/manage-environments.html) (also works with [micromamba](https://mamba.readthedocs.io/en/latest/user_guide/micromamba.html)):
```bash
conda create -n ManiVaultPythonPluginBuild -y python=3.11 poetry=2.0
```
2. Pass `Python_EXECUTABLE` and `ManiVaultDir` to cmake:
    - Define `ManiVault_DIR` pointing to the ManiVault cmake file folder, e.g. `YOUR_LOCAL_PATH/install/cmake/mv`
    - Define `Python_EXECUTABLE` pointing to the python exe, e.g. 
        - venv: `YOUR_LOCAL_PATH/AppData/Local/Programs/Python/ManiVaultPythonPluginBuild/Scripts/python.exe`
        - Conda: `YOUR_LOCAL_PATH/AppData/Local/miniconda3/envs/ManiVaultPythonPluginBuild/python.exe` (Windows), `YOUR_LOCAL_PATH/miniconda3/envs/ManiVaultPythonPluginBuild/bin/python` (Linux)
        - On Linux you might need to activate the environment for CMake to find the correct Python.

    (Alternatively, define `Python_ROOT_DIR`, the root directory of a Python installation.)

3. (On Windows) Use [vcpkg](https://github.com/microsoft/vcpkg): define `CMAKE_TOOLCHAIN_FILE="[YOURPATHTO]/vcpkg/scripts/buildsystems/vcpkg.cmake"` to install the OpenSSL dependency. You'll also need to set `MV_JUPYTER_USE_VCPKG=ON`.

This projects builds two ManiVault plugins, a communication bridge `JupyterPlugin311` between the ManiVault application and Python as well as a `JupyterLauncher` plugin, which is used to launch a Python kernel and Jupyter notebook.
Additionally two python modules are build `mvstudio_data` and `mvstudio_kernel` that used two pass data between Python and ManiVault. 

It is advisable to use a different environment during building these plugins and during runtime.
Building the two python modules requires `poetry` which is automatically installed in the `Python_EXECUTABLE` environment (see `requirements.txt`).
`mvstudio_data` and `mvstudio_kernel` have to be installed when first starting the JupyterPlugin in a python environment.
ManiVault will ask you if this should be performed automatically.

You may build multiple Jupyter Plugins, e.g. `JupyterPlugin311` and `JupyterPlugin312` for different python version. 
The Jupyter Launcher will list all available plugins during runtime.

## Dependencies

Windows (if using as a subproject, i.e. the manifest file cannot be used for automatically installing dependencies):
```
.\vcpkg.exe install openssl:x64-windows-static-md
```

Linux:
```
sudo apt install libtbb-dev libsodium-dev libssl-dev uuid-dev
```

You may set the option `XEUS_BUILD_STATIC_DEPENDENCIES` to `ON` if you installed `libuuid` in your build environment, e.g. with `mamba install libuuid -c conda-forge`, see [xeus README](https://github.com/jupyter-xeus/xeus?tab=readme-ov-file#building-from-source).

Mac:
```
brew --prefix libomp
```

## Usage

You can use any local python environment with this plugin to interact with ManiVault. 
You need to provide a path to a python interpreter in the ManiVault settings for this. 
When starting the plugin (via the toolbar on the bottom), you are asked to provide a path the the python environment.
Alternatively, go to `File` -> `Settings` -> `Plugin: Jupyter Launcher` -> `Python interpreter` and navigate to your local python interpreter. (These can be the same as used for building or any other).

Upon the first start with a given interpreter, two communication modules are automatically installed, which will help translate between ManiVault's data hierarchy and your Python script.

To access data from ManiVault:
```python
import mvstudio.data
h = mvstudio.data.Hierarchy()
print(h)
```

Upon starting the Jupyter integration, a notebook is automatically opened.
When executing scripts, an UI is automatically populated to provide user input for any argument that the script may expect.

### Limitations

Currently, ManiVault cannot make a notebook connection and script execution available at the same time. 

### Running on Linux

When using a non-system python interpreter, we have to make sure that the environment variable `LD_PRELOAD` contains the interpreter path that we intend to use in order to [prevent a conflict between the system libstdc++ and the python environment libstdc++](https://www.scivision.dev/conda-python-libstdc-/):

#### Using a Conda environment

Before starting the application (assuming your local environment uses Python 3.12):
```
conda activate your_local_env
CURRENT_PYTHON_PATH=$(find ${CONDA_PREFIX} -name libpython3.12* 2>/dev/null | head -n 1)
conda env config vars set LD_PRELOAD=$CURRENT_PYTHON_PATH --name your_local_env
conda deactivate && conda activate your_local_env
```

#### Using a Mamba environment

Before starting the application (assuming your local environment uses Python 3.12):
```
micromamba activate your_local_env
CURRENT_PYTHON_PATH=$(find ${CONDA_PREFIX} -name libpython3.12* 2>/dev/null | head -n 1)
echo -e "{\"env_vars\": {\"LD_PRELOAD\": \"${CURRENT_PYTHON_PATH}\"}}" >> ${CONDA_PREFIX}/conda-meta/state
micromamba deactivate && micromamba activate your_local_env
```

## Use of Jupyter logo

This plugin kernel based on the Xeus kernel is compatible with Jupyter software. As such we use the unmodified Jupyter logo to make this clear to end users. This usage follows the rules set out in the [Jupyter documentation](https://jupyter.org/governance/trademarks.html#uses-that-never-require-approval).

## Kernel architecture

The kernel relies on Jupyter-Xeus components to expose a python environment. 

The architecture of the kernel is based on [Slicer/SlicerJupyter](https://github.com/Slicer/SlicerJupyter). The correspondence is shown in the table below.

ManiVault JupyterPlugin | Slicer SlicerJupyter extension
---| --- 
JupyterXeusKernel | qSlicerJupyterKernelModule 
JupyterPlugin | qSlicerJupyterKernelModuleWidget


