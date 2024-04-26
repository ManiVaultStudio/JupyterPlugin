import mvstudio_core
from typing import Generator, Self
import numpy as np
from enum import Enum

class ImageItem:
    def __init__(self, host):
        self._host = host

class ClusterItem:
    def __init__(self, host):
        self._host = host

ImageItems = list[ImageItem]
ClusterItems = list[ClusterItem]

class DataHierarchyItem:
    ItemType = Enum('ItemType', ['Image', 'Points', 'Cluster'])

    def __init__(self, guid, name):
        self._guid = guid
        self._name = name
        self._datasetid = None
        self._selected = False
        self._children = []
        self._data = None
        self._type = None
        self._addChildren()
        self._setType()

    def _addChildren(self):
        for child_guid in mvstudio_core.get_item_children(self._guid):
            item_name = mvstudio_core.get_item_name(self._guid)
            self._children.append(DataHierarchyItem(child_guid, item_name))

    def _setType(self):
        match mvstudio_core.get_data_type(self._guid):
            case mvstudio_core.DataItemType.Image:
                self._type = DataHierarchyItem.ItemType.Image
            case mvstudio_core.DataItemType.Points:
                self._type = DataHierarchyItem.ItemType.Points
            case mvstudio_core.DataItemType.Cluster:
                self._type = DataHierarchyItem.ItemType.Cluster 
        self._setData()

    def _setData(self):
        data = mvstudio_core.get_data_for_item(self._guid)
        # dependant on the data type 
        # post process to a more pythonic representation
        match self._type:
            case DataHierarchyItem.ItemType.Points:
                self._data = data
            case DataHierarchyItem.ItemType.Image:
                self._data = data
            case DataHierarchyItem.ItemType.Cluster:
                self._data = data
    
    def children(self) -> Generator[Self, None, None]:
        """Generator for iterating over any children of this DataHierarchyItem.

        Yields:
            A child DataHierarchyItem

        """
        index = 0
        while index < len(self._children):
            yield self._children[index]
            index += 1

    def getItem(self, guid) -> Self:
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
    
    def getItemByName(self, name) -> Self:
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

    @property
    def type(self) -> ItemType:
        return self._type

    @property
    def guid(self) -> str:
        return self._guid
    
    @property
    def name(self) -> str:
        return self._name

    @property
    def type(self) -> str:
        """Return the data type"""
        return mvstudio_core.get_item_type(self.guid)

    @property
    def rawname(self) -> str:
        """Return the raw name"""
        return mvstudio_core.get_item_rawname(self.guid)

    @property
    def rawsize(self) -> int:
        """Return the raw data size in bytes"""
        return mvstudio_core.get_item_rawsize(self.guid)
    
    @property
    def numdimensions(self) -> int:
        """Return the number of dimensions"""
        return mvstudio_core.get_item_numdimensions(self.guid)
    
    @property
    def numpoints(self) -> int:
        """Return the numper of points"""
        return mvstudio_core.get_item_numpoints(self.guid)

    def getImage(self) -> np.ndarray:
        """If this is an image return the image data in a numpy array
            otherwise return None
        """
        return mvstudio_core.get_image_item(self.guid)


    def __str__(self) -> str:
        children_strs = [f'DataHierarchyItem id: {self.guid}, display name: {self.name}']
        for child in self.children():
            children_strs.append(" " + str(child))

        return '\n'.join(children_strs)
    
    def __repr__(self) -> str:
        children_reprs = [f'DataHierarchyItem({self.guid}, {self.name})']
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
        for guid in self._top_level:
            item_name = mvstudio_core.get_item_name(guid)
            self._hierarchy.append(DataHierarchyItem(guid, item_name))   

    def refresh(self): 
        """Rebuild the DataHierarchy tree from the MvStudio
        """
        self._refresh()

    def getItem(self, guid) -> DataHierarchyItem:
        """Return the DataHierarchy Item corresponding
        to the guid 
        """
        for item in self.children():
            result = item.getItem(guid)
            if result is not None:
                return result
        return None

    def getItemByName(self, name) -> DataHierarchyItem:
        """Return the DataHierarchy Item corresponding
        to the guid 
        """
        for item in self.children():
            result = item.getItemByName(name)
            if result is not None:
                return result
        return None

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
