# LLAE Crypto Module Documentation

The LLAE crypto module provides cryptographic functionality through a combination of Lua and native C++ implementations using the mbedTLS library.

## Basic Usage

```lua
local crypto = require 'llae.crypto'
```

## Available Components

### Message Digest (MD)

The module provides message digest functionality through the `md` submodule. The `md` class provides the following methods:

```lua
-- Create a new message digest instance
local md_instance = crypto.md.new('SHA256') -- or other algorithm

-- Update the digest with data
md_instance:update(data)

-- Finalize and get the hash
local hash = md_instance:finish()
```

Example usage with MD5:
```lua
-- Create an MD5 hash instance
local md5 = crypto.md.new('MD5')
md5:update("Hello, World!")
local hash = md5:finish()
```

Available algorithms for md.new():
- 'NONE'
- 'MD5'
- 'SHA1'
- 'SHA224'
- 'SHA256'
- 'SHA384'
- 'SHA512'
- 'RIPEMD160'

### Available Algorithms

The following message digest algorithms are supported:

- `MD_NONE` - No algorithm
- `MD_MD5` - MD5 message digest
- `MD_SHA1` - SHA-1 message digest
- `MD_SHA224` - SHA-224 message digest
- `MD_SHA256` - SHA-256 message digest
- `MD_SHA384` - SHA-384 message digest
- `MD_SHA512` - SHA-512 message digest
- `MD_RIPEMD160` - RIPEMD-160 message digest

### HMAC

HMAC (Hash-based Message Authentication Code) functionality is available through the `hmac` submodule:

```lua
-- Create new HMAC instance
local hmac_instance = crypto.hmac.new(algorithm) -- e.g. 'SHA256', 'MD5', etc.

-- Initialize HMAC with key
hmac_instance:start(key)

-- Update HMAC with data
hmac_instance:update(data)

-- Get HMAC result
local hmac_result = hmac_instance:finish()

-- Reset HMAC for reuse
hmac_instance:reset()
```

### Cipher Operations

The cipher module provides symmetric encryption/decryption functionality:

```lua
-- Create new cipher instance
local cipher = crypto.cipher.new(algorithm) -- e.g. 'AES-256-CBC'

-- Set encryption key and mode
cipher:set_key(key, crypto.ENCRYPT) -- or crypto.DECRYPT for decryption
cipher:set_iv(iv) -- Set initialization vector
cipher:set_padding(padding_mode) -- Set padding mode

-- Process data
local encrypted = cipher:update(data)
local final_block = cipher:finish()

-- Reset cipher for reuse
cipher:reset()
```

Available padding modes:
- `PADDING_PKCS7`
- `PADDING_ONE_AND_ZEROS`
- `PADDING_ZEROS_AND_LEN`
- `PADDING_ZEROS`
- `PADDING_NONE`

### Public Key Cryptography

#### RSA Operations

RSA operations are available through the Public Key (`pk`) module:

```lua
-- Create public key instance
local pk = crypto.pk.new()

-- Parse public key
pk:parse_public_key(key_data)

-- Parse private key (with optional password for encrypted keys)
pk:parse_private_key(key_data, password)

-- Get RSA context (if the key is RSA)
local rsa = pk:get_rsa()

-- Encrypt data with the public key
local encrypted = pk:encrypt(data, random_generator)

-- Decrypt data with the private key
local decrypted = pk:decrypt(encrypted, random_generator)
```

Encryption uses the public key; decryption requires a private key loaded with `parse_private_key`. Both `encrypt` and `decrypt` are asynchronous and need a yielding context. The optional `random_generator` is a `crypto.random` instance; if omitted, a default RNG is used.

Supported padding types:
- `RSA_PKCS_V15`
- `RSA_PKCS_V21`

#### ECP (Elliptic Curve Point) Operations

The ECP module provides elliptic curve cryptography support:

```lua
-- Create ECP instance
local ecp = crypto.ecp.new()

-- Generate key pair
local public_key, private_key = ecp:gen_keypair()

-- Sign data
local r, s = ecp:ecdsa_sign(data, private_key, hash_algorithm)

-- Verify signature
local is_valid = ecp:ecdsa_verify(data, r, s, public_key)
```

Point formats:
- `ECP_PF_COMPRESSED`
- `ECP_PF_UNCOMPRESSED`

### Random Number Generation

The module provides cryptographically secure random number generation:

```lua
-- Create entropy source
local entropy = crypto.entropy.new()

-- Create random number generator
local rng = crypto.random.new(entropy)

-- Update RNG with additional entropy
rng:update(additional_data)
```

### Additional Components

- `bignum` - Big number arithmetic operations
- `ecp` - Elliptic Curve operations
- `ecp_point` - Elliptic Curve Point operations
- `pk` - Public Key operations
- `entropy` - Entropy source for random number generation
- `random` - Cryptographically secure random number generation

### Asynchronous Operations

Many cryptographic operations in the module are asynchronous and require a yielding context:

- HMAC operations (start, update, finish)
- Cipher operations (update, finish)
- Message Digest operations (update, finish)
- CRC32 calculation

Example of asynchronous operation:
```lua
-- This must be run in a yielding context
local result = await(crypto.hmac.new('SHA256'):start(key))
```

### Error Handling

All cryptographic operations may return errors in the format:
```lua
nil, "operation failed, code: X, description"
```

Always check return values and handle errors appropriately.

### Security Best Practices

1. Use secure random number generation for keys and IVs
2. Always verify signatures with proper algorithms
3. Use appropriate key sizes for the chosen algorithms
4. Handle sensitive data (keys, private information) securely
5. Use appropriate padding modes for your use case
6. Keep cryptographic operations in a yielding context when required

## Examples

```lua
-- Create and use MD5 hash
local md5 = crypto.md.new('MD5')
-- Use other crypto operations as needed
```

For more detailed examples, please refer to the examples directory in the LLAE project. 