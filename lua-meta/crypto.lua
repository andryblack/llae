---@meta crypto

---The crypto module provides cryptographic functionality through mbedTLS library.
---Supports message digests, HMAC, cipher operations, public key cryptography, and random number generation.
---@class crypto
local crypto = {}

--- Calculates CRC32 checksum of the input data.
---@param start integer Starting CRC32 value
---@param data string|llae.buffer_base The input data to calculate CRC32 for
---@return integer? The CRC32 checksum on success
---@return string? Error message if calculation fails
function crypto.crc32(start, data) end

---Message digest functionality for hashing data using various algorithms.
---Supported algorithms: 'NONE', 'MD5', 'SHA1', 'SHA224', 'SHA256', 'SHA384', 'SHA512', 'RIPEMD160'
---@class crypto.md
local md = {}

--- Creates a new message digest instance with the specified algorithm.
---@param algorithm string The hash algorithm to use (e.g., 'MD5', 'SHA256', 'SHA1')
---@return crypto.md? The message digest instance on success
---@return string? Error message if creation fails
function md.new(algorithm) end

--- Updates the digest with additional data.
---@param data string|llae.buffer_base The data to add to the digest
---@return boolean? True on success
---@return string? Error message if update fails
function md:update(data) end

--- Finalizes the digest and returns the hash result.
---@return llae.buffer? The hash result on success
---@return string? Error message if finalization fails
function md:finish() end

---HMAC (Hash-based Message Authentication Code) functionality for message authentication.
---@class crypto.hmac
local hmac = {}

--- Creates a new HMAC instance with the specified hash algorithm.
---@param algorithm string The hash algorithm to use (e.g., 'SHA256', 'MD5', 'SHA1')
---@return crypto.hmac? The HMAC instance on success
---@return string? Error message if creation fails
function hmac.new(algorithm) end

--- Initializes HMAC with the provided key.
---@param key string|llae.buffer_base The HMAC key
---@return boolean? True on success
---@return string? Error message if initialization fails
function hmac:start(key) end

--- Updates HMAC with additional data.
---@param data string|llae.buffer_base The data to add to the HMAC
---@return boolean? True on success
---@return string? Error message if update fails
function hmac:update(data) end

--- Finalizes HMAC and returns the authentication code.
---@return llae.buffer? The HMAC result on success
---@return string? Error message if finalization fails
function hmac:finish() end

--- Resets HMAC for reuse with a new key.
---@return boolean? True on success
---@return string? Error message if reset fails
function hmac:reset() end

---Symmetric encryption/decryption functionality using various cipher algorithms.
---Supports algorithms like 'AES-256-CBC', 'AES-128-CBC', etc.
---@class crypto.cipher
local cipher = {}

--- Creates a new cipher instance with the specified algorithm.
---@param algorithm string The cipher algorithm to use (e.g., 'AES-256-CBC')
---@return crypto.cipher? The cipher instance on success
---@return string? Error message if creation fails
function cipher.new(algorithm) end

--- Sets the initialization vector (IV) for the cipher.
---@param iv string|llae.buffer_base The initialization vector
---@return boolean? True on success
---@return string? Error message if setting IV fails
function cipher:set_iv(iv) end

--- Sets the encryption/decryption key for the cipher.
---@param key string|llae.buffer_base The cipher key
---@return boolean? True on success
---@return string? Error message if setting key fails
function cipher:set_key(key) end

--- Sets the padding mode for the cipher.
---@param padding integer Padding mode (PADDING_PKCS7, PADDING_ONE_AND_ZEROS, PADDING_ZEROS_AND_LEN, PADDING_ZEROS, PADDING_NONE)
---@return boolean? True on success
---@return string? Error message if setting padding fails
function cipher:set_padding(padding) end

--- Resets the cipher for reuse.
---@return boolean? True on success
---@return string? Error message if reset fails
function cipher:reset() end

--- Processes data through the cipher (encrypt/decrypt).
---@param data string|llae.buffer_base The data to process
---@return llae.buffer? The processed data on success
---@return string? Error message if processing fails
function cipher:update(data) end

--- Finalizes the cipher operation and returns any remaining data.
---@return llae.buffer? The final processed data on success
---@return string? Error message if finalization fails
function cipher:finish() end

---Big number arithmetic operations for cryptographic computations.
---@class crypto.bignum
local bignum = {}

--- Creates a new big number instance.
---@return crypto.bignum A new big number instance
function bignum.new() end

--- Sets the value of the big number.
---@param value integer|string|crypto.bignum The value to set
function bignum:set(value) end

--- Adds another big number to this one (in-place).
---@param other crypto.bignum The big number to add
function bignum:self_add(other) end

--- Multiplies this big number by another (in-place).
---@param other crypto.bignum The big number to multiply by
function bignum:self_mul(other) end

--- Subtracts another big number from this one (in-place).
---@param other crypto.bignum The big number to subtract
function bignum:self_sub(other) end

--- Multiplies this big number by another and returns the result.
---@param other crypto.bignum The big number to multiply by
---@return crypto.bignum The result of the multiplication
function bignum:mul(other) end

--- Divides this big number by another.
---@param other crypto.bignum The divisor
---@return crypto.bignum? The quotient on success
---@return crypto.bignum? The remainder on success
---@return string? Error message if division fails
function bignum:div(other) end

--- Adds another big number to this one and returns the result.
---@param other crypto.bignum The big number to add
---@return crypto.bignum The result of the addition
function bignum:add(other) end

--- Subtracts another big number from this one and returns the result.
---@param other crypto.bignum The big number to subtract
---@return crypto.bignum The result of the subtraction
function bignum:sub(other) end

--- Writes the big number to a string representation.
---@param format string? The output format (e.g., 'hex', 'binary')
---@return string? The string representation on success
---@return string? Error message if writing fails
function bignum:write(format) end

--- Reads a big number from string data.
---@param data string|llae.buffer_base The input data
---@param format string? The input format (e.g., 'hex', 'binary')
---@return boolean? True on success
---@return string? Error message if reading fails
function bignum:read(data, format) end

--- Checks if this big number is less than another.
---@param other crypto.bignum The big number to compare with
---@return boolean? True if this number is less than the other
---@return string? Error message if comparison fails
function bignum:less(other) end

--- Checks if this big number is less than or equal to another.
---@param other crypto.bignum The big number to compare with
---@return boolean? True if this number is less than or equal to the other
---@return string? Error message if comparison fails
function bignum:lequal(other) end

--- Converts the big number to a string representation.
---@return string? The string representation on success
---@return string? Error message if conversion fails
function bignum:tostring() end

---@class crypto.ecp_point
---Elliptic Curve Point operations for cryptographic computations.
local ecp_point = {}

--- Creates a new elliptic curve point instance.
---@return crypto.ecp_point A new elliptic curve point instance
function ecp_point.new() end

--- Sets the point to zero (point at infinity).
function ecp_point:set_zero() end

--- Checks if the point is zero (point at infinity).
---@return boolean True if the point is zero
function ecp_point:is_zero() end

--- Compares this point with another point.
---@param other crypto.ecp_point The point to compare with
---@return boolean True if the points are equal
function ecp_point:cmp(other) end

--- Reads a point from string data.
---@param data string|llae.buffer_base The input data containing the point
---@return boolean? True on success
---@return string? Error message if reading fails
function ecp_point:read_string(data) end

---@class crypto.ecp
---Elliptic Curve operations for cryptographic computations including ECDSA signing and verification.
local ecp = {}

--- Creates a new elliptic curve instance with the specified group.
---@param group_id string The elliptic curve group identifier
---@return crypto.ecp? The elliptic curve instance on success
---@return string? Error message if creation fails
function ecp.new(group_id) end

--- Reads a point from binary data.
---@param data string|llae.buffer_base The binary data containing the point
---@return crypto.ecp_point? The elliptic curve point on success
---@return string? Error message if reading fails
function ecp:point_read_binary(data) end

--- Writes a point to binary format.
---@param point crypto.ecp_point The elliptic curve point to write
---@param format integer? Point format (ECP_PF_COMPRESSED or ECP_PF_UNCOMPRESSED)
---@return llae.buffer? The binary representation on success
---@return string? Error message if writing fails
function ecp:point_write_binary(point, format) end

--- Checks if a public key point is valid.
---@param point crypto.ecp_point The public key point to validate
---@return boolean True if the public key is valid
function ecp:check_pubkey(point) end

--- Checks if a private key is valid.
---@param privkey crypto.bignum The private key to validate
---@return boolean True if the private key is valid
function ecp:check_privkey(privkey) end

--- Verifies an ECDSA signature.
---@param hash string|llae.buffer_base The hash of the data that was signed
---@param signature string|llae.buffer_base The signature to verify
---@param pubkey crypto.ecp_point The public key to verify against
---@return boolean? True if the signature is valid
---@return string? Error message if verification fails
function ecp:ecdsa_verify(hash, signature, pubkey) end

--- Signs data using ECDSA.
---@param hash string|llae.buffer_base The hash of the data to sign
---@param privkey crypto.bignum The private key to sign with
---@return llae.buffer? The signature on success
---@return string? Error message if signing fails
function ecp:ecdsa_sign(hash, privkey) end

--- Generates a new key pair.
---@param random_data string|llae.buffer_base? Additional random data for key generation
---@return crypto.bignum? The private key on success
---@return crypto.ecp_point? The public key on success
---@return string? Error message if generation fails
function ecp:gen_keypair(random_data) end

--- Generates a new private key.
---@param random_data string|llae.buffer_base? Additional random data for key generation
---@return crypto.bignum? The private key on success
---@return string? Error message if generation fails
function ecp:gen_privkey(random_data) end

--- Generates a public key from a private key.
---@param privkey crypto.bignum The private key
---@return crypto.ecp_point? The public key on success
---@return string? Error message if generation fails
function ecp:gen_pubkey(privkey) end

--- Sets additional random data for cryptographic operations.
---@param data string|llae.buffer_base Additional random data
---@return boolean? True on success
---@return string? Error message if setting fails
function ecp:set_random_data(data) end

---@class crypto.pk
---Public Key operations for RSA and other public key cryptography.
local pk = {}

--- Creates a new public key instance.
---@return crypto.pk A new public key instance
function pk.new() end

--- Parses a public key from key data.
---@param key_data string|llae.buffer_base The public key data
---@return boolean? True on success
---@return string? Error message if parsing fails
function pk:parse_public_key(key_data) end

--- Gets the name/type of the public key.
---@return string? The key name on success
---@return string? Error message if getting name fails
function pk:get_name() end

--- Gets the RSA context if the key is an RSA key.
---@return crypto.rsa_base? The RSA context on success
---@return string? Error message if getting RSA context fails
function pk:get_rsa() end

--- Encrypts data using the public key.
---@param data string|llae.buffer_base The data to encrypt
---@return llae.buffer? The encrypted data on success
---@return string? Error message if encryption fails
function pk:encrypt(data) end

---@class crypto.rsa_base
---RSA-specific operations for public key cryptography.
local rsa_base = {}

--- Sets the padding mode for RSA operations.
---@param padding integer Padding mode (RSA_PKCS_V15 or RSA_PKCS_V21)
---@return boolean? True on success
---@return string? Error message if setting padding fails
function rsa_base:set_padding(padding) end

---@class crypto.entropy
---Entropy source for cryptographically secure random number generation.
local entropy = {}

--- Creates a new entropy source instance.
---@return crypto.entropy A new entropy source instance
function entropy.new() end

---@class crypto.random
---Cryptographically secure random number generation.
local random = {}

--- Creates a new random number generator.
---@return crypto.random? The random number generator on success
---@return string? Error message if creation fails
function random.new() end

---@param entropy crypto.entropy? Optional entropy source
---@param pers string? Optional persistent string
---@return boolean? True on success
---@return string? Error message if randomization fails
function random:seed(entropy,pers) end

--- Updates the random number generator with additional entropy.
---@param data string|llae.buffer_base Additional entropy data
---@return boolean? True on success
---@return string? Error message if update fails
function random:update(data) end

-- Constants
crypto.ECP_PF_COMPRESSED = 0
crypto.ECP_PF_UNCOMPRESSED = 0
crypto.DECRYPT = 0
crypto.ENCRYPT = 0
crypto.PADDING_PKCS7 = 0
crypto.PADDING_ONE_AND_ZEROS = 0
crypto.PADDING_ZEROS_AND_LEN = 0
crypto.PADDING_ZEROS = 0
crypto.PADDING_NONE = 0
crypto.RSA_PKCS_V15 = 0
crypto.RSA_PKCS_V21 = 0

crypto.md = md
crypto.hmac = hmac
crypto.cipher = cipher
crypto.bignum = bignum
crypto.ecp_point = ecp_point
crypto.ecp = ecp
crypto.pk = pk
crypto.rsa_base = rsa_base
crypto.entropy = entropy
crypto.random = random

--- HKDF (HMAC-based Key Derivation Function) for key derivation.
---@param md string|integer The message digest algorithm to use
---@param salt llae.buffer_base|string? The salt value
---@param info llae.buffer_base|string? The info value
---@param key llae.buffer_base|string? The key value
---@param osize integer The output size
---@return llae.buffer? The HKDF result on success
---@return string? Error message if HKDF fails
function crypto.hkdf(md, salt, info, key, osize) end


return crypto
