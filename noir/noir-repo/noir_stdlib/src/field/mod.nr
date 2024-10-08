mod bn254;
use bn254::lt as bn254_lt;

impl Field {
    /// Asserts that `self` can be represented in `bit_size` bits.
    ///
    /// # Failures
    /// Causes a constraint failure for `Field` values exceeding `2^{bit_size}`.
    pub fn assert_max_bit_size(self, bit_size: u32) {
        crate::assert_constant(bit_size);
        assert(bit_size < modulus_num_bits() as u32);
        self.__assert_max_bit_size(bit_size);
    }

    #[builtin(apply_range_constraint)]
    fn __assert_max_bit_size(self, bit_size: u32) {}

    /// Decomposes `self` into its little endian bit decomposition as a `[u1]` slice of length `bit_size`.
    /// This slice will be zero padded should not all bits be necessary to represent `self`.
    /// 
    /// # Failures
    /// Causes a constraint failure for `Field` values exceeding `2^{bit_size}` as the resulting slice will not
    /// be able to represent the original `Field`.
    ///
    /// # Safety
    /// Values of `bit_size` equal to or greater than the number of bits necessary to represent the `Field` modulus
    /// (e.g. 254 for the BN254 field) allow for multiple bit decompositions. This is due to how the `Field` will
    /// wrap around due to overflow when verifying the decomposition.
    pub fn to_le_bits(self: Self, bit_size: u32) -> [u1] {
        crate::assert_constant(bit_size);
        self.__to_le_bits(bit_size)
    }

    /// Decomposes `self` into its big endian bit decomposition as a `[u1]` slice of length `bit_size`.
    /// This slice will be zero padded should not all bits be necessary to represent `self`.
    /// 
    /// # Failures
    /// Causes a constraint failure for `Field` values exceeding `2^{bit_size}` as the resulting slice will not
    /// be able to represent the original `Field`.
    ///
    /// # Safety
    /// Values of `bit_size` equal to or greater than the number of bits necessary to represent the `Field` modulus
    /// (e.g. 254 for the BN254 field) allow for multiple bit decompositions. This is due to how the `Field` will
    /// wrap around due to overflow when verifying the decomposition.
    pub fn to_be_bits(self: Self, bit_size: u32) -> [u1] {
        crate::assert_constant(bit_size);
        self.__to_be_bits(bit_size)
    }

    /// See `Field.to_be_bits`
    #[builtin(to_le_bits)]
    fn __to_le_bits(self, _bit_size: u32) -> [u1] {}

    /// See `Field.to_le_bits`
    #[builtin(to_be_bits)]
    fn __to_be_bits(self, bit_size: u32) -> [u1] {}

    /// Decomposes `self` into its little endian byte decomposition as a `[u8]` slice of length `byte_size`.
    /// This slice will be zero padded should not all bytes be necessary to represent `self`.
    /// 
    /// # Failures
    /// Causes a constraint failure for `Field` values exceeding `2^{8*byte_size}` as the resulting slice will not
    /// be able to represent the original `Field`.
    ///
    /// # Safety
    /// Values of `byte_size` equal to or greater than the number of bytes necessary to represent the `Field` modulus
    /// (e.g. 32 for the BN254 field) allow for multiple byte decompositions. This is due to how the `Field` will
    /// wrap around due to overflow when verifying the decomposition.
    pub fn to_le_bytes(self: Self, byte_size: u32) -> [u8] {
        self.to_le_radix(256, byte_size)
    }

    /// Decomposes `self` into its big endian byte decomposition as a `[u8]` slice of length `byte_size`.
    /// This slice will be zero padded should not all bytes be necessary to represent `self`.
    /// 
    /// # Failures
    /// Causes a constraint failure for `Field` values exceeding `2^{8*byte_size}` as the resulting slice will not
    /// be able to represent the original `Field`.
    ///
    /// # Safety
    /// Values of `byte_size` equal to or greater than the number of bytes necessary to represent the `Field` modulus
    /// (e.g. 32 for the BN254 field) allow for multiple byte decompositions. This is due to how the `Field` will
    /// wrap around due to overflow when verifying the decomposition.
    pub fn to_be_bytes(self: Self, byte_size: u32) -> [u8] {
        self.to_be_radix(256, byte_size)
    }

    pub fn to_le_radix(self: Self, radix: u32, result_len: u32) -> [u8] {
        crate::assert_constant(radix);
        crate::assert_constant(result_len);
        self.__to_le_radix(radix, result_len)
    }

    pub fn to_be_radix(self: Self, radix: u32, result_len: u32) -> [u8] {
        crate::assert_constant(radix);
        crate::assert_constant(result_len);
        self.__to_be_radix(radix, result_len)
    }

    // `_radix` must be less than 256
    #[builtin(to_le_radix)]
    fn __to_le_radix(self, radix: u32, result_len: u32) -> [u8] {}

    #[builtin(to_be_radix)]
    fn __to_be_radix(self, radix: u32, result_len: u32) -> [u8] {}

    // Returns self to the power of the given exponent value.
    // Caution: we assume the exponent fits into 32 bits
    // using a bigger bit size impacts negatively the performance and should be done only if the exponent does not fit in 32 bits
    pub fn pow_32(self, exponent: Field) -> Field {
        let mut r: Field = 1;
        let b = exponent.to_le_bits(32);

        for i in 1..33 {
            r *= r;
            r = (b[32-i] as Field) * (r * self) + (1 - b[32-i] as Field) * r;
        }
        r
    }

    // Parity of (prime) Field element, i.e. sgn0(x mod p) = 0 if x ∈ {0, ..., p-1} is even, otherwise sgn0(x mod p) = 1.
    pub fn sgn0(self) -> u1 {
        self as u1
    }

    pub fn lt(self, another: Field) -> bool {
        if crate::compat::is_bn254() {
            bn254_lt(self, another)
        } else {
            lt_fallback(self, another)
        }
    }
}

#[builtin(modulus_num_bits)]
pub comptime fn modulus_num_bits() -> u64 {}

#[builtin(modulus_be_bits)]
pub comptime fn modulus_be_bits() -> [u1] {}

#[builtin(modulus_le_bits)]
pub comptime fn modulus_le_bits() -> [u1] {}

#[builtin(modulus_be_bytes)]
pub comptime fn modulus_be_bytes() -> [u8] {}

#[builtin(modulus_le_bytes)]
pub comptime fn modulus_le_bytes() -> [u8] {}

// Convert a 32 byte array to a field element by modding
pub fn bytes32_to_field(bytes32: [u8; 32]) -> Field {
    // Convert it to a field element
    let mut v = 1;
    let mut high = 0 as Field;
    let mut low = 0 as Field;

    for i in 0..16 {
        high = high + (bytes32[15 - i] as Field) * v;
        low = low + (bytes32[16 + 15 - i] as Field) * v;
        v = v * 256;
    }
    // Abuse that a % p + b % p = (a + b) % p and that low < p
    low + high * v
}

fn lt_fallback(x: Field, y: Field) -> bool {
    let num_bytes = (modulus_num_bits() as u32 + 7) / 8;
    let x_bytes = x.to_le_bytes(num_bytes);
    let y_bytes = y.to_le_bytes(num_bytes);
    let mut x_is_lt = false;
    let mut done = false;
    for i in 0..num_bytes {
        if (!done) {
            let x_byte = x_bytes[num_bytes - 1 - i] as u8;
            let y_byte = y_bytes[num_bytes - 1 - i] as u8;
            let bytes_match = x_byte == y_byte;
            if !bytes_match {
                x_is_lt = x_byte < y_byte;
                done = true;
            }
        }
    }
    x_is_lt
}

