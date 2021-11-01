# aniFileExtractor

This is a simple and not production ready script to extract images from `.ani` files.

The project has the goal to be a single script which can even convert `.ani` files directly to `.xcf` files so that they should be used on X11 Linux desktop environments.

## Research

To be able to write this script the following information was researched and is necessary to understand the code:

### `.ani` file structure

`.ani` files use the generic `RIFF` container format which means the file starts with `RIFF` which is followed by the type of container which should be `ACON`.

| Offset | 0   | 1   | 2   | 3   | 4   | 5   | 6   | 7   |
| ------ | --- | --- | --- | --- | --- | --- | --- | --- |
| Data   | `R` | `I` | `F` | `F` | `A` | `C` | `O` | `N` |

Then in no particular order:

| 4 Byte Block Name  | 4 Byte Length Endian | Data Purpose |
| ------------------ | -------------------- | ------------ |
| `'INAM'`             | LE                   | Name string  |
| `'IART'`             | LE                   | Artist string |
| `'LIST'`             | LE                   | It should contain all icons |
| `'fram'`             | ---                  | Block that seems to be inside `LIST` data before `icon` blocks but has no length |
| `'icon'`             | LE                   | Contains the image data |
| `'anih'`             | LE                   | ANIH information which should be 36 Byte long |
| `'rate'`             | LE                   | TODO |
| `'seq '`             | LE                   | TODO |

The `anih` information:

| Offset  | Size | Endian | Purpose  |
| ------- | ---- | ------ | -------- |
| 0       | 4    | LE     | cbSizeOf: Number of unique Icons in this cursor |
| 4       | 4    | LE     | cFrames: Number of bytes in AniHeader |
| 8       | 4    | LE     | cSteps: Number of Blits before the animation cycles |
| 12      | 4    | ---    | cx: reserved, must be zero |
| 16      | 4    | ---    | cy: reserved, must be zero |
| 20      | 4    | ---    | cBitCount: reserved, must be zero |
| 24      | 4    | ---    | cPlanes: reserved, must be zero |
| 28      | 4    | LE     | JifRate: Default Jiffies (1/60th of a second) if rate chunk not present), TODO |
| 32      | 4    | ---    | Animation Flag, TODO |

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

| Offset  | Size | Endian | Purpose  |
| ------- | ---- | ------ | -------- |
| 0       | 2    | ---    | Reserved. Must always be 0. |
| 2       | 2    | LE     | Specifies image type: 1 for icon (.ICO) image, 2 for cursor (.CUR) image. Other values are invalid. |
| 4       | 2    | LE     | Specifies number of images in the file. |

##### Directory

| Offset  | Size | Endian | Purpose |
| ------- | ---- | ------ | ------- |
| 0       | 1    | LE     | Specifies image width in pixels. Can be any number between 0 and 255. Value 0 means image width is 256 pixels. |
| 1       | 1    | LE     | Specifies image height in pixels. Can be any number between 0 and 255. Value 0 means image height is 256 pixels. |
| 2       | 1    | LE     | Specifies number of colors in the color palette. Should be 0 if the image does not use a color palette. |
| 3       | 1    | ---    | Reserved. Should be 0. |
| 4       | 2    | LE     | In ICO format: Specifies color planes. Should be 0 or 1.[Notes 3] In CUR format: Specifies the horizontal coordinates of the hotspot in number of pixels from the left. |
| 6       | 2    | LE     | In ICO format: Specifies bits per pixel. In CUR format: Specifies the vertical coordinates of the hotspot in number of pixels from the top. |
| 8       | 4    | LE     | Specifies the size of the image's data in bytes |
| 12      | 4    | LE     | Specifies the offset of BMP or PNG data from the beginning of the ICO/CUR file |

TODO

### `.png` file structure

Sources:

- https://docs.fileformat.com/image/png/

The first 8 bytes of a `.png` file always contain the following values (represented in little-endian byte order):

| Offset | 0   | 1  | 2  | 3  | 4  | 5  | 6  | 7  |
| ------ | --- | -- | -- | -- | -- | -- | -- | -- |
| Data   | 137 | 80 | 78 | 71 | 13 | 10 | 26 | 10 |

Then a list of chunks is following.

#### Chunk

| Offset# | Size | Endian | Purpose
| ------- | ---- | ------ | ------- |
| 0       | 4    | BE     | Number of byte data of chunk (=x) |
| 4       | 4    | ---    | Chunk type (Chars) |
| 8       | x    | ?      | Chunk data |
| 8 + x   | 4    | ?      | CRC (Cyclic Redundancy Checksum) - not provided if the chunk data length is 0 |

TODO

### Big/Little Endian numbers

TODO
