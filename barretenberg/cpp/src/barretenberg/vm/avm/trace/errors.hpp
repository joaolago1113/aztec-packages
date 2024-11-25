#pragma once

#include <cstdint>

namespace bb::avm_trace {

enum class AvmError : uint32_t {
    NO_ERROR,
    TAG_ERROR,
    ADDR_RES_TAG_ERROR,
    REL_ADDR_OUT_OF_RANGE,
    DIV_ZERO,
    PARSING_ERROR,
    ENV_VAR_UNKNOWN,
    CONTRACT_INST_MEM_UNKNOWN,
    RADIX_OUT_OF_BOUNDS,
};

} // namespace bb::avm_trace