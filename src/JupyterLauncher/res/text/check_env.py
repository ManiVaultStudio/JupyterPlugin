import importlib.util

def main():
    exit(importlib.util.find_spec('MVPluginManager') is None)

if __name__ == "__main__":
    main()