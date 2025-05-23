// === AUDIT STATUS ===
// internal:    { status: not started, auditors: [], date: YYYY-MM-DD }
// external_1:  { status: not started, auditors: [], date: YYYY-MM-DD }
// external_2:  { status: not started, auditors: [], date: YYYY-MM-DD }
// =====================

#pragma once
#include "barretenberg/polynomials/legacy_polynomial.hpp"
#include <string>
#include <unordered_map>

namespace bb {

template <typename Fr> class PolynomialStoreWasm {
  private:
    using Polynomial = bb::LegacyPolynomial<Fr>;
    std::unordered_map<std::string, size_t> size_map;

  public:
    void put(std::string const& key, Polynomial&& value);

    Polynomial get(std::string const& key);
};

} // namespace bb
