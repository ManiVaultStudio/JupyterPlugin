import mvstudio_core
from typing import Generator, Self
import numpy as np
import numpy.typing as npt
import typing
import pandas as pd
from enum import Enum

class ImageItem:
    def __init__(self, host):
        self._host = host
class ClusterItem:
    def __init__(self, host):
        self._host = host
ImageItems = list[ImageItem]
ClusterItems = list[ClusterItem]
ClusterType = dict[str, npt.NDArray[np.uint32]]

class DataHierarchyCluster:
    """_summary_
    Wraps a pandas.DataFram containing the cluster indexes and names
    appending ids and colors as additional metadata properties
    """

    def __init__(self, clusters: ClusterType, col_ids: list[str], col_colors: list[tuple[float, float, float, float]]):
        """_summary_

        Args:
            clusters (ClusterType): A dict (keys are cluster names) of numpy uint32 indexes
            col_ids (list): List of cluster guids
            col_colors (list): List of RGBA color float tuples  ()

        Returns:
            _type_: _description_

        Yields:
            _type_: _description_
        """
        self._clusters = clusters
        self._ids = col_ids
        self._colors = col_colors

    @property
    def clusters(self) -> ClusterType:
        return self._clusters
    
    @property
    def col_ids(self) -> list[str]:
        return self._ids
    
    @property
    def colors(self) -> list[str]:
        return self._colors
    
class DataHierarchyItem:
    ItemType = Enum('ItemType', ['Image', 'Points', 'Cluster'])

    def __init__(self, guid_tuple, name, hierarchy_id):
        self._guid_tuple = guid_tuple  # contains the item guid and dataset guid
        self._name = name
        self._hierarchy_id = hierarchy_id
        self._datasetid = None
        self._selected = False
        self._children = []
        self._data = None
        self._type = None
        self._setType()
        self._addChildren()

    def _addChildren(self):
        guidTuples = mvstudio_core.get_item_children(self.datasetId)
        child_id = 1
        for childGuidTuple in guidTuples:
            item_name = mvstudio_core.get_item_name(self.datasetId)
            self._children.append(DataHierarchyItem(childGuidTuple, item_name, self._hierarchy_id + [child_id]))
            child_id += 1

    def _setType(self):
        match mvstudio_core.get_data_type(self.datasetId):
            case mvstudio_core.DataItemType.Image:
                self._type = DataHierarchyItem.ItemType.Image
            case mvstudio_core.DataItemType.Points:
                self._type = DataHierarchyItem.ItemType.Points
            case mvstudio_core.DataItemType.Cluster:
                self._type = DataHierarchyItem.ItemType.Cluster 
        self._setData()

    def _setData(self):
        if self._type is DataHierarchyItem.ItemType.Points:
            data = mvstudio_core.get_data_for_item(self.datasetId)
        # dependant on the data type 
        # post process to a more pythonic representation
        match self._type:
            case DataHierarchyItem.ItemType.Points:
                self._data = data
            case DataHierarchyItem.ItemType.Image:
                self._data = None
            case DataHierarchyItem.ItemType.Cluster:
                self._data = None
    
    def children(self) -> Generator[Self, None, None]:
        """Generator for iterating over any children of this DataHierarchyItem.

        Yields:
            A child DataHierarchyItem

        """
        index = 0
        while index < len(self._children):
            yield self._children[index]
            index += 1

    def getItem(self, guid) -> Self | None:
        """Return the DataHierarchy Item corresponding
        to the guid. It can be the current item or a child item.
        """
        if self._guid == guid:
            return self
        
        for child in self.children():
            item = child.getItem(guid)
            if item is not None:
                return item
        return None
    
    def getItemByName(self, name) -> Self | None:
        """Return the DataHierarchy Item corresponding
        to the name. It can be the current item or a child item.
        """
        if self._name == name:
            return self
        
        for child in self.children():
            item = child.getItemByName(name)
            if item is not None:
                return item
        return None
    
    # Yes this is very stupid! 
    # Will optimize later...
    def getItemByIndex(self, index: list[int]) -> Self | None:
        """Return an item matching the index or NOne

        Args:
            index (list[int]): _description_

        Returns:
            Self: _description_
        """
        item = None
        if self._hierarchy_id == index:
            return self
        
        for child in self.children():
            item = child.getItemByIndex(index)
            if item is not None:
                break
        return item

    
    @property
    def points(self) -> np.ndarray:
        return mvstudio_core.get_data_for_item(self.datasetId)

    @property
    def type(self) -> ItemType:
        return self._type

    @property
    def guid(self) -> str:
        return self._guid_tuple[0]
    
    @property
    def datasetId(self) -> str:
        return self._guid_tuple[1]
    
    @property
    def name(self) -> str:
        return self._name

    @property
    def type(self) -> str:
        """Return the data type"""
        return self._type

    @property
    def rawname(self) -> str:
        """Return the raw name"""
        return mvstudio_core.get_item_rawname(self.datasetId)

    @property
    def rawsize(self) -> int:
        """Return the raw data size in bytes"""
        return mvstudio_core.get_item_rawsize(self.datasetId)
    
    @property
    def numdimensions(self) -> int:
        """Return the number of dimensions"""
        return mvstudio_core.get_item_numdimensions(self.datasetId)
    
    @property
    def numpoints(self) -> int:
        """Return the numper of points"""
        return mvstudio_core.get_item_numpoints(self.datasetId)

    @property
    def image(self) -> np.ndarray:
        """If this is an image return the image data in a numpy array
            otherwise return None
        """
        # information is in the image metadata
        if self.type is not DataHierarchyItem.ItemType.Image:
            return None
        id = mvstudio_core.find_image_dataset(self.datasetId)
        if len(id) > 0:
            size = mvstudio_core.get_image_dimensions(id)
            array =  mvstudio_core.get_image_item(self.datasetId)
            return array.reshape(size[0], size[1], -1)
        return None
    
    @property
    def cluster(self) -> DataHierarchyCluster:
        if self.type is not DataHierarchyItem.ItemType.Cluster:
            return None
        (indexes, names, colors, ids) = mvstudio_core.get_cluster(self.datasetId)
        clusterDict = dict(zip(names, indexes))
        return DataHierarchyCluster(clusterDict, ids, colors)  


    def __str__(self) -> str:
        children_strs = [f'DataHierarchyItem id: {self._hierarchy_id}, dataset id: {self.datasetId}, display name: {self.name}, type: {self.type}']
        for child in self.children():
            children_strs.append(" " + str(child))

        return '\n'.join(children_strs)
    
    def __repr__(self) -> str:
        children_reprs = [f'DataHierarchyItem({self._hierarchy_id}, {self.datasetId}, {self.name}, {repr(self.type)})']
        for child in self.children():
            children_reprs.append(" " + repr(child))

        return '\n'.join(children_reprs)
    
    @property
    def images(self) -> ImageItems:
        return self._images

    @property
    def clusters(self) -> ClusterItems:
        return self._clusters
      
class DataHierarchy:
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
            self._hierarchy.append(DataHierarchyItem(guid_tuple, item_name, [hierarchy_id]))   
            hierarchy_id += 1

    def refresh(self): 
        """Rebuild the DataHierarchy tree from the MvStudio
        """
        self._refresh()

    def getItem(self, guid: str) -> DataHierarchyItem:
        """Return the DataHierarchy Item corresponding
        to the guid 
        """
        for item in self.children():
            result = item.getItem(guid)
            if result is not None:
                return result
        return None

    def getItemByName(self, name: str) -> DataHierarchyItem:
        """Return the DataHierarchy Item corresponding
        to the guid 
        """
        for item in self.children():
            result = item.getItemByName(name)
            if result is not None:
                return result
        return None
    
    def getItemByIndex(self, index: list[int]) -> DataHierarchyItem:
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



    def children(self) -> Generator[DataHierarchyItem, None, None]:
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

