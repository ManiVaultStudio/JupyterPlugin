import mvstudio_core
from .item import Item
import numpy.typing as npt
import numpy as np

from clusterbase import Cluster

class ClusterMixin:
    """Mixin for Item to create a Cluster Item"""
    @property
    def cluster(self) -> Cluster:
        if self.type is not Item.ItemType.Cluster:
            return None
        (indexes, names, colors, ids) = mvstudio_core.get_cluster(self.datasetId)
        return Cluster(names, indexes, ids, colors)  

class ClusterItem(ClusterMixin, Item):
    """
    ClusterItem adds the cluster property to the
    basic Item type 
    """
    def __init__(self, guid_tuple, item_name, hierarchy_id):
        super().__init__(guid_tuple, item_name, hierarchy_id)