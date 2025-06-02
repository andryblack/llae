# LLAE Compression Module

The compression module provides a high-level interface for data compression and decompression using zlib. It wraps the lower-level archive module functionality into easy-to-use functions.

## Usage

```lua
local compression = require 'llae.compression'
```

## Functions

### compression.deflate(data)

Compresses the input data using zlib deflate algorithm.

**Parameters:**
- `data` (string): The input data to compress

**Returns:**
- `string`: The compressed data on success
- `nil, error_message`: If compression fails, returns nil and an error message

**Example:**
```lua
local compressed = compression.deflate("Hello World!")
```

### compression.inflate(data)

Decompresses data that was compressed using zlib deflate algorithm.

**Parameters:**
- `data` (string): The compressed data to inflate

**Returns:**
- `string`: The decompressed data on success
- `nil, error_message`: If decompression fails, returns nil and an error message

**Example:**
```lua
local decompressed = compression.inflate(compressed_data)
```

## Low-Level API

The module also exposes low-level compression functionality through the following classes and functions:

### Compression Classes

- `zcompress` - Base compression class
- `zcompress_deflate_read` - Deflate compression stream reader
- `zcompress_gzip_read` - Gzip compression stream reader
- `zcompress_to_stream` - Stream-based compression writer

### Decompression Classes

- `zuncompress` - Base decompression class
- `zuncompress_inflate_read` - Inflate decompression stream reader
- `zuncompress_gzip_read` - Gzip decompression stream reader
- `zuncompress_to_stream` - Stream-based decompression writer

### Factory Functions

- `new_deflate_read()` - Creates a new deflate compression reader
- `new_gzip_read()` - Creates a new gzip compression reader
- `new_deflate_to_stream()` - Creates a new deflate compression stream writer
- `new_inflate_read()` - Creates a new inflate decompression reader
- `new_gunzip_read()` - Creates a new gzip decompression reader
- `new_inflate_to_stream()` - Creates a new inflate decompression stream writer

### Memory Management

- `get_allocated()` - Returns the current amount of memory allocated by the compression module
