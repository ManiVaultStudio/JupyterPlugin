import argparse
import subprocess
import os

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("connectionfile", help="absolute path to connection file")
    args = parser.parse_args()
    os.environ["MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE"] = args.connectionfile
    # python -m jupyterlab_server --ServerApp.kernel_manager_class=MVJupyterPluginManager.ExternalMappingKernelManager --KernelProvisionerFactory.default_provisioner_name=mvjupyterplugin-existing-provisioner
    subprocess.run(["python", "-m", "jupyterlab_server", "--ServerApp.kernel_manager_class=MVJupyterPluginManager.ExternalMappingKernelManager", "--KernelProvisionerFactory.default_provisioner_name=mvjupyterplugin-existing-provisioner"])

if __name__ == '__main__':
    main()