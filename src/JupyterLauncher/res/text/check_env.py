import importlib.util
from importlib.metadata import version as get_version
import sys

def main(required_version):
    print(f"Searching for ManiVault communication modules version {required_version}")

    dependency_modules = ["mvstudio.kernel", "mvstudio.data"]

    for dependency_module in dependency_modules:
        if importlib.util.find_spec(dependency_module) is None:
            print(f"Could not find {dependency_module}")
            exit(1)

        module_version = get_version(dependency_module)
        print(f"Found {dependency_module} version {module_version}")

        if module_version!= required_version:
            print(f"{dependency_module} version {module_version} is not {required_version}")
            exit(1)

    exit(0)

if __name__ == "__main__":
    required_version = str(sys.argv[1])
    main(required_version)