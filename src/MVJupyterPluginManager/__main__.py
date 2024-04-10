import argparse
import subprocess
from pathlib import Path
import os

def main():
    """
    Start the jupyter lab with the package configuration file.
    The user muat supply the the path tothe connection file
    that contains the connection configuration written by the 
    ManiVault Studio JupyterPlugin
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("connectionfile", help="Required absolute path to connection file, must match the path in JupyterLauncher")
    args = parser.parse_args()
    os.environ["MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE"] = args.connectionfile
    # The provisioner name is requested via the kernel metadata
    # and the mapping is inthe MVPluginKerenelManager entry point
    # config_path = Path(Path(__file__).parent, "config", "jupyter_notebook_config.py")
    # python -m jupyterlab_server --ServerApp.kernel_manager_class=MVJupyterPluginManager.ExternalMappingKernelManager --KernelProvisionerFactory.default_provisioner_name=mvjupyterplugin-existing-provisioner
    print(f"Running jupyter lab with configuration at {args.connectionfile}")
    subprocess.run([
        "python", 
        "-m", 
        "jupyter",
        "lab", 
        f"--config={args.connectionfile}"
    ])

if __name__ == '__main__':
    main()