import mvstudio_core 

def makeItem(hierarchy, guid_tuple, item_name, hierarchy_id):
    """Factory method for items"""
    match mvstudio_core.get_data_type(guid_tuple[1]):
        case mvstudio_core.DataItemType.Image:
            from .image import ImageItem 
            return ImageItem(hierarchy, guid_tuple, item_name, hierarchy_id)
        case mvstudio_core.DataItemType.Points:
            from .item import Item
            return Item(hierarchy, guid_tuple, item_name, hierarchy_id)
        case mvstudio_core.DataItemType.Cluster:
            from .cluster import ClusterItem
            return ClusterItem(hierarchy, guid_tuple, item_name, hierarchy_id)