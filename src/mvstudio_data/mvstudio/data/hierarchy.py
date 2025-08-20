import mvstudio_core
import numpy as np
import warnings
from typing import Generator, Self
from .item import Item
from .factory import makeItem
         
class Hierarchy:
    def __init__(self):
        self._dataType = "Unknown"
        self._hierarchy = []
        self._refresh()

    def _refresh(self) -> None:
        self._hierarchy = []
        self._top_level = mvstudio_core.get_top_level_guids()
        self._build_hierarchy() 

    def _build_hierarchy(self) -> None: 
        hierarchy_id = 1
        for guid_tuple in self._top_level:
            item_name = mvstudio_core.get_item_name(guid_tuple[1])
            self._hierarchy.append(makeItem(self, guid_tuple, item_name, [hierarchy_id]))   
            hierarchy_id += 1

    def refresh(self): 
        """Rebuild the DataHierarchy tree from the MvStudio
        """
        self._refresh()

    def getItem(self, itemId: str) -> Item:
        """Return the Item corresponding
        to the data hierarchy guid 
        """
        for item in self.children():
            result = item.getItem(itemId)
            if result is not None:
                return result
        return None

    def getItemByDataID(self, datasetId: str) -> Item:
        """Return the Item corresponding
        to the data set guid 
        """
        for item in self.children():
            result = item.getItemByDataID(datasetId)
            if result is not None:
                return result
        return None

    def getItemByName(self, name: str) -> Item:
        """Return the Item corresponding
        to the data name 
        """
        for item in self.children():
            result = item.getItemByName(name)
            if result is not None:
                return result
        return None
    
    def getItemByIndex(self, index: list[int]) -> Item:
        """Get an item from the data hierarchy by 
        providing an index. And index is a list 
        e.g. [2,1,2] where each number in the list 
        represent the index-number in a hierarchy level
        To see the assigned index-numbers do print(DataHierarchy()) 

        Args:
            index_list: _description_

        Returns:
            DataHierarchyItem: _description_
        """
        item = None
        for child in self.children():
            item = child.getItemByIndex(index)
            if item is not None:
                break
        return item
    
    def addPointsItem(self, data: np.ndarray, name: str, parentDataId : str = "", dimensionNames : list[str] = list()) -> Item|None:
        """Add a points data item

        Args: 
            data: The numpy array contain the point data, of size N * M for N points with M dimensions
            name: A name for the point data set
            parentDataId: (optional) Dataset ID of the parent in the data hierarchy (does not derive the new data form the parent). If empty, the data will be placed at root without parent
            dimensionNames: (optional) List of dimension names. If empty, dimensions will be numbered

        Returns:
            Item|None: Data hierarchy item reference ManiVault
        """
        assert data.ndim == 2, "Data array must be two-dimensional (num_points, num_dims)"

        if dimensionNames:
          assert data.shape[1] == len(dimensionNames), "Dimensionnames must be of size num_dims"

        datasetId = mvstudio_core.add_new_data(data, name, parentDataId, dimensionNames)
        
        if len(datasetId) == 0:
            warnings.warn("Could not add item", RuntimeWarning)
            return None
        else:
            self._refresh()
            return self.getItemByDataID(datasetId)
        
    def addImageItem(self, data: np.ndarray, name: str, dimensionNames : list[str] = list()) -> Item|None:
        """Add an image data item

        Args:
            data (np.ndarray): A numpy array representing the image of shape (x, y, dims)
            names: A name for the image item.
            dimensionNames: (optional) List of dimension names. If empty, dimensions will be numbered

        Returns:
            Item|None: Data hierarchy item reference ManiVault
        """
        assert data.ndim == 2 or data.ndim == 3, "Data array must be of shape (x, y, dims)"

        if dimensionNames:
          if data.ndim == 2:
            assert 1 == len(dimensionNames), "Dimensionnames must be 1 (grayscale image)"
          else:
            assert data.shape[2] == len(dimensionNames), "Dimensionnames must be of size num_dims"
        
        datasetId = mvstudio_core.add_new_image(data, name, dimensionNames)
        
        if len(datasetId) == 0:
            warnings.warn("Could not add item", RuntimeWarning)
            return None
        else:
            self._refresh()
            return self.getItemByDataID(datasetId)

    def addClusterItem(self, parent: str, indices: list[np.ndarray], name: str, **kwargs):
        """Add an cluster data set

        Args:
            indices: A list of arrays containing the data indices of each clusters
            parent: GUID of the parent data set
            names (optional): A list of string containing names for each cluster
            name: A name for the cluster item
            colors (optional): A list of arrays containing colors of each clusters
        Returns:
            Item|None: Data hierarchy item reference ManiVault
        """
        names = kwargs.get('names', [])
        colors = kwargs.get('colors', [])
    
        datasetId = mvstudio_core.add_new_cluster(parent, indices, names, colors, name)

        if len(datasetId) == 0:
            warnings.warn("Could not add item", RuntimeWarning)
            return None
        else:
            self._refresh()
            return self.getItemByDataID(datasetId)

    def children(self) -> Generator[Item, None, None]:
            """Generator for iterating over any children of this DataHierarchyItem.

            Yields:
                A top-level DataHierarchyItem

            """
            index = 0
            while index < len(self._top_level):
                yield self._hierarchy[index]
                index += 1

    def __str__(self) -> str:
        result =  f'ManiVault data items:' 
        for item in self.children():
            result = result + '\n' + str(item)
        return result
    
    def __repr__(self) -> str:
        result =  f'DataHierarchy(\n' 
        for item in self.children():
            result = result + '\n' + repr(item)
        return result

