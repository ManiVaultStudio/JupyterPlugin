import mvstudio_core
from typing import Generator, Self, Any
import numpy as np
import numpy.typing as npt
from enum import Enum
from .factory import makeItem
from .clusterbase import Cluster

class Item:
    """
    Item is a Points item and at the same time it
    forms the basis for other item types (Image, Cluster)
    which are created using Mixins
    """
    ItemType = Enum('ItemType', ['Image', 'Points', 'Cluster'])
            
    def __init__(self, hierarchy, guid_tuple, name, hierarchy_id):
        self._hierarchy = hierarchy
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
            child_name = mvstudio_core.get_item_name(childGuidTuple[1])
            self._children.append(makeItem(self._hierarchy, childGuidTuple, child_name, self._hierarchy_id + [child_id]))
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

    def getItem(self, itemId : str) -> Self | None:
        """Return the DataHierarchy Item corresponding to the given
        hierarchy item ID. It can be the current item or a child item.
        """
        if self.itemId == itemId:
            return self
        
        for child in self.children():
            item = child.getItem(itemId)
            if item is not None:
                return item
        return None
    
    def getItemByDataID(self, datasetId : str) -> Self | None:
        """Return the DataHierarchy Item corresponding to the given
        dataset ID. It can be the current item or a child item.
        """
        if self.datasetId == datasetId:
            return self
        
        for child in self.children():
            item = child.getItemByDataID(datasetId)
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
    
    def getSelection(self) -> np.ndarray:
        """Get the selection for a given dataset, specified using its dataset ID.
        """
        return mvstudio_core.get_selection_for_item(self.datasetId)

    def setSelection(self, selectionIDs : np.ndarray) -> None:
        """Set the selection for a given dataset, specified using its dataset ID.
        """
        return mvstudio_core.set_selection_for_item(self.datasetId, selectionIDs)

    @property
    def points(self) -> np.ndarray:
        return mvstudio_core.get_data_for_item(self.datasetId)

    @property
    def type(self) -> ItemType:
        return self._type

    @property
    def itemId(self) -> str:
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
    def properties(self) -> list[str]:
        """Return the names of available properties"""
        return mvstudio_core.get_item_properties(self.datasetId)

    def getProperty(self, name : str) -> Any:
        """Return the property with a given name"""
        return mvstudio_core.get_item_property(self.datasetId, name)

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
