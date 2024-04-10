import importlib.util
from importlib.metadata import version

def main():
    print("Searching for MVJupyterPluginManager")
    
    if importlib.util.find_spec('MVJupyterPluginManager') is None:
        exit(1)
    if version('MVJupyterPluginManager') != '0.4.5':
        exit(1)
    exit(0)

if __name__ == "__main__":
    main()