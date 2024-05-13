import mvstudio_core
from .item import Item
import numpy.typing as npt
import numpy as np

Image = npt.NDArray[np.float32]
Images = list[Image] 

class ImageMixin:
    """Mixin for Item to create an Image Item"""
    @property
    def image(self) -> np.ndarray:
        """If this is an image return the image data in a numpy array
            otherwise return None
        """
        # information is in the image metadata
        if self.type is not Item.ItemType.Image:
            return None
        id = mvstudio_core.find_image_dataset(self.datasetId)
        if len(id) > 0:
            size = mvstudio_core.get_image_dimensions(id)
            array =  mvstudio_core.get_image_item(self.datasetId)
            return array.reshape(size[0], size[1], -1)
        return None 

class ImageItem(ImageMixin, Item):
    def __init__(self, guid_tuple, name, hierarchy_id):
        super().__init__(guid_tuple, name, hierarchy_id)