# aniFileExtractor

This is a simple and not production ready script to extract images from `.ani` files.

The project has the goal to be a single script which can even convert `.ani` files directly to `.xcf` files so that they should be used on X11 Linux desktop environments.

## Research

To be able to write this script the following information was researched and is necessary to understand the code:

### `.ani` file structure

TODO

### `.ico`/`.cur` file structure

Sources:

- https://en.wikipedia.org/wiki/ICO_(file_format)
- https://docs.fileformat.com/image/ico/

All values in `.ico`/`.cur` files are represented in little-endian byte order (default in programming).

#### Structure

1. **Icon Header**: Stores general information about the ICO file
2. **Directory[1..n]**: Stores general information about every image in the file
3. **Icon[1..n]**: The actual data for the image sin old AND/XOR DIB format or newer PNG format

##### Icon Header

| Offset# | Size | Purpose |
| ------- | ---- | ------- |
| 0       | 2    | Reserved. Must always be 0. |
| 2       | 2    | Specifies image type: 1 for icon (.ICO) image, 2 for cursor (.CUR) image. Other values are invalid. |
| 4       | 2    | Specifies number of images in the file. |

##### Directory

| Offset# | Size | Purpose
| ------- | ---- | ------- |
| 0       | 1    | Specifies image width in pixels. Can be any number between 0 and 255. Value 0 means image width is 256 pixels. |
| 1       | 1    | Specifies image height in pixels. Can be any number between 0 and 255. Value 0 means image height is 256 pixels. |
| 2       | 1    | Specifies number of colors in the color palette. Should be 0 if the image does not use a color palette. |
| 3       | 1    | Reserved. Should be 0.[Notes 2] |
| 4       | 2    | In ICO format: Specifies color planes. Should be 0 or 1.[Notes 3] In CUR format: Specifies the horizontal coordinates of the hotspot in number of pixels from the left. |
| 6       | 2    | In ICO format: Specifies bits per pixel. [Notes 4] In CUR format: Specifies the vertical coordinates of the hotspot in number of pixels from the top. |
| 8       | 4    | Specifies the size of the image's data in bytes |
| 12      | 4    | Specifies the offset of BMP or PNG data from the beginning of the ICO/CUR file |

TODO

### `.png` file structure

TODO
