import mvstudio_core
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
            self._hierarchy.append(makeItem(guid_tuple, item_name, [hierarchy_id]))   
            hierarchy_id += 1

    def refresh(self): 
        """Rebuild the DataHierarchy tree from the MvStudio
        """
        self._refresh()

    def getItem(self, guid: str) -> Item:
        """Return the Item corresponding
        to the guid 
        """
        for item in self.children():
            result = item.getItem(guid)
            if result is not None:
                return result
        return None

    def getItemByName(self, name: str) -> Item:
        """Return the Item corresponding
        to the guid 
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

