# mvstudio/__init__.py
import importlib.util
__all__ = (
    "data_hierarchy",
    "jupyter_kernel"
)

from . import jupyter_kernel
from . import data_hierarchy

# This import only works in the kernel environment 
# that supplies mvstudio.data
#if importlib.util.find_spec('mvstudio_core') is not None:
#    from . import data_hierarchy
