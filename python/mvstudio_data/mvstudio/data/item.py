import mvstudio_core
from typing import Generator, Self
import numpy as np
import numpy.typing as npt
from enum import Enum
from .factory import makeItem
from clusterbase import Clusters

class Item:
    """
    Item is a Points item and at the same time it
    forms the basis for other item types (Image, Cluster)
    which are created using Mixins
    """
    ItemType = Enum('ItemType', ['Image', 'Points', 'Cluster'])
            
    def __init__(self, guid_tuple, name, hierarchy_id):
        self._guid_tuple = guid_tuple  # contains the item guid and dataset guid
        self._name = name
        self._hierarchy_id = hierarchy_id
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
            self._children.append(makeItem(childGuidTuple, item_name, self._hierarchy_id + [child_id]))
            child_id += 1

    def _setType(self):
        match mvstudio_core.get_data_type(self.datasetId):
            case mvstudio_core.DataItemType.Image:
                self._type = Item.ItemType.Image
            case mvstudio_core.DataItemType.Points:
                self._type = Item.ItemType.Points
            case mvstudio_core.DataItemType.Cluster:
                self._type = Item.ItemType.Cluster 
        self._setData()

    def _setData(self):
        if self._type is Item.ItemType.Points:
            data = mvstudio_core.get_data_for_item(self.datasetId)
        # dependant on the data type 
        # post process to a more pythonic representation
        match self._type:
            case Item.ItemType.Points:
                self._data = data
            case Item.ItemType.Image:
                self._data = None
            case Item.ItemType.Cluster:
                self._data = None
    
    def children(self) -> Generator[Self, None, None]:
        """Generator for iterating over any children of this Item.

        Yields:
            A child Item

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
    
    def addClusterItem(self, clusters: Clusters) -> Self | None:
        """Add a clusteritem as a child of this points item

        Args:
            cluster (Cluster): A set of clusters that matches the point items in terms of indexes

        Returns:
            Self | None: _description_
        """
        for cluster in clusters:
            if not all(c < self.numpoints for c in cluster.clusters):
                return None
        
        
    
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

    def __str__(self) -> str:
        children_strs = [f'{"  "*(len(self._hierarchy_id)-1)}Item id: {self._hierarchy_id}, dataset id: {self.datasetId}, display name: {self.name}, type: {self.type}']
        for child in self.children():
            children_strs.append(str(child))

        return '\n'.join(children_strs)
    
    def __repr__(self) -> str:
        children_reprs = [f'Item({self._hierarchy_id}, {self.datasetId}, {self.name}, {repr(self.type)})']
        for child in self.children():
            children_reprs.append(repr(child))

        return '\n'.join(children_reprs)
