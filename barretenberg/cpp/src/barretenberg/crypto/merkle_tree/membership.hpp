// === AUDIT STATUS ===
// internal:    { status: not started, auditors: [], date: YYYY-MM-DD }
// external_1:  { status: not started, auditors: [], date: YYYY-MM-DD }
// external_2:  { status: not started, auditors: [], date: YYYY-MM-DD }
// =====================

#pragma once
#include "barretenberg/crypto/merkle_tree/memory_store.hpp"
#include "barretenberg/crypto/merkle_tree/merkle_tree.hpp"
#include "barretenberg/stdlib/hash/pedersen/pedersen.hpp"
#include "barretenberg/stdlib/primitives/byte_array/byte_array.hpp"
#include "barretenberg/stdlib/primitives/field/field.hpp"
#include "hash_path.hpp"

namespace bb::crypto::merkle_tree {

template <typename Builder> using bit_vector = std::vector<bb::stdlib::bool_t<Builder>>;
template <typename Builder> using field_t = bb::stdlib::field_t<Builder>;
template <typename Builder> using bool_t = bb::stdlib::bool_t<Builder>;
/**
 * Computes the new merkle root if the subtree is correctly inserted at a specified index in a Merkle tree.
 *
 * @param hashes: The hash path from any leaf in the subtree to the root, it doesn't matter if this hash path is
 * computed before or after updating the tree,
 * @param value: The value of the subtree root,
 * @param index: The index of any leaf in the subtree,
 * @param at_height: The height of the subtree,
 * @param is_updating_tree: set to true if we're updating the tree.
 * @tparam Builder: type of builder.
 *
 * @see Check full documentation: https://hackmd.io/2zyJc6QhRuugyH8D78Tbqg?view
 */
template <typename Builder>
field_t<Builder> compute_subtree_root(hash_path<Builder> const& hashes,
                                      field_t<Builder> const& value,
                                      bit_vector<Builder> const& index,
                                      size_t at_height,
                                      bool const is_updating_tree = false)
{
    auto current = value;
    for (size_t i = at_height; i < hashes.size(); ++i) {
        // get the parity bit at this level of the tree (get_bit returns bool so we know this is 0 or 1)
        bool_t<Builder> path_bit = index[i];

        // reconstruct the two inputs we need to hash
        // if `path_bit = false`, we know `current` is the left leaf and `hashes[i].second` is the right leaf
        // if `path_bit = true`, we know `current` is the right leaf and `hashes[i].first` is the left leaf
        // We don't need to explicitly check that hashes[i].first = current iff !path bit , or that hashes[i].second =
        // current iff path_bit If either of these does not hold, then the final computed merkle root will not match
        field_t<Builder> left = field_t<Builder>::conditional_assign(path_bit, hashes[i].first, current);
        field_t<Builder> right = field_t<Builder>::conditional_assign(path_bit, current, hashes[i].second);
        if (is_updating_tree) {
            current = bb::stdlib::pedersen_hash<Builder>::hash({ left, right }, 0);
        } else {
            current = bb::stdlib::pedersen_hash<Builder>::hash_skip_field_validation({ left, right }, 0);
        }
    }

    return current;
}

/**
 * Checks if the subtree is correctly inserted at a specified index in a Merkle tree.
 *
 * @param root: The root of the latest state of the merkle tree,
 * @param hashes: The hash path from any leaf in the subtree to the root, it doesn't matter if this hash path is
 * computed before or after updating the tree,
 * @param value: The value of the subtree root,
 * @param index: The index of any leaf in the subtree,
 * @param at_height: The height of the subtree,
 * @param is_updating_tree: set to true if we're updating the tree.
 * @tparam Builder: type of builder.
 *
 * @see Check full documentation: https://hackmd.io/2zyJc6QhRuugyH8D78Tbqg?view
 */
template <typename Builder>
bool_t<Builder> check_subtree_membership(field_t<Builder> const& root,
                                         hash_path<Builder> const& hashes,
                                         field_t<Builder> const& value,
                                         bit_vector<Builder> const& index,
                                         size_t at_height,
                                         bool const is_updating_tree = false)
{
    return (compute_subtree_root(hashes, value, index, at_height, is_updating_tree) == root);
}

/**
 * Asserts if the subtree is correctly inserted at a specified index in a Merkle tree.
 *
 * @param root: The root of the latest state of the merkle tree,
 * @param hashes: The hash path from any leaf in the subtree to the root, it doesn't matter if this hash path is
 * computed before or after updating the tree,
 * @param value: The value of the subtree root,
 * @param index: The index of any leaf in the subtree,
 * @param at_height: The height of the subtree,
 * @param is_updating_tree: set to true if we're updating the tree,
 * @param msg: error message.
 * @tparam Builder: type of builder.
 */
template <typename Builder>
void assert_check_subtree_membership(field_t<Builder> const& root,
                                     hash_path<Builder> const& hashes,
                                     field_t<Builder> const& value,
                                     bit_vector<Builder> const& index,
                                     size_t at_height,
                                     bool const is_updating_tree = false,
                                     std::string const& msg = "assert_check_subtree_membership")
{
    auto exists = check_subtree_membership(root, hashes, value, index, at_height, is_updating_tree);
    exists.assert_equal(true, msg);
}

/**
 * Checks if a value is correctly inserted at a specified leaf in a Merkle tree.
 *
 * @param root: The root of the updated merkle tree,
 * @param hashes: The hash path from the given index, it doesn't matter if this hash path is
 * computed before or after updating the tree,
 * @param value: The value of the leaf,
 * @param index: The index of the leaf in the tree,
 * @param is_updating_tree: set to true if we're updating the tree.
 * @tparam Builder: type of builder.
 */
template <typename Builder>
bool_t<Builder> check_membership(field_t<Builder> const& root,
                                 hash_path<Builder> const& hashes,
                                 field_t<Builder> const& value,
                                 bit_vector<Builder> const& index,
                                 bool const is_updating_tree = false)
{
    return check_subtree_membership(root, hashes, value, index, 0, is_updating_tree);
}

/**
 * Asserts if a value is correctly inserted at a specified leaf in a Merkle tree.
 *
 * @param root: The root of the updated merkle tree,
 * @param hashes: The hash path from the given index, it doesn't matter if this hash path is
 * computed before or after updating the tree,
 * @param value: The value of the leaf,
 * @param index: The index of the leaf in the tree,
 * @param is_updating_tree: set to true if we're updating the tree,
 * @param msg: error message.
 * @tparam Builder: type of builder.
 */
template <typename Builder>
void assert_check_membership(field_t<Builder> const& root,
                             hash_path<Builder> const& hashes,
                             field_t<Builder> const& value,
                             bit_vector<Builder> const& index,
                             bool const is_updating_tree = false,
                             std::string const& msg = "assert_check_membership")
{
    auto exists = check_membership(root, hashes, value, index, is_updating_tree);
    exists.assert_equal(true, msg);
}

/**
 * Asserts if old and new state of the tree is correct after updating a single leaf.
 *
 * @param new_root: The root of the updated merkle tree,
 * @param new_value: The new value of the leaf,
 * @param old_root: The root of the merkle tree before it was updated,
 * @param old_hashes: The hash path from the given index, it doesn't matter if this hash path is
 * computed before or after updating the tree,
 * @param old_value: The value of the leaf before it was updated with new_value,
 * @param index: The index of the leaf in the tree,
 * @param msg: error message.
 * @tparam Builder: type of builder.
 */
template <typename Builder>
void update_membership(field_t<Builder> const& new_root,
                       field_t<Builder> const& new_value,
                       field_t<Builder> const& old_root,
                       hash_path<Builder> const& old_hashes,
                       field_t<Builder> const& old_value,
                       bit_vector<Builder> const& index,
                       std::string const& msg = "update_membership")
{
    // Check that the old_value, is in the tree given by old_root, at index.
    assert_check_membership(old_root, old_hashes, old_value, index, false, msg + "_old_value");

    // Check that the new_value, is in the tree given by new_root, at index.
    assert_check_membership(new_root, old_hashes, new_value, index, true, msg + "_new_value");
}

/**
 * Asserts if the state transitions on updating multiple existing leaves with new values.
 *
 * @param old_root: The root of the merkle tree before it was updated,
 * @param new_roots: New roots after each existing leaf was updated,
 * @param new_values: The new values that are inserted in the existing leaves,
 * @param old_values: The values of the existing leaves that were updated,
 * @param old_paths: The hash path from the given index right before a given existing leaf is updated,
 * @param old_indicies: Indices of the existing leaves that need to be updated,
 * @tparam Builder: type of builder.
 */
template <typename Builder>
field_t<Builder> update_memberships(field_t<Builder> old_root,
                                    std::vector<field_t<Builder>> const& new_roots,
                                    std::vector<field_t<Builder>> const& new_values,
                                    std::vector<field_t<Builder>> const& old_values,
                                    std::vector<hash_path<Builder>> const& old_paths,
                                    std::vector<bit_vector<Builder>> const& old_indicies)
{
    for (size_t i = 0; i < old_indicies.size(); i++) {
        update_membership(
            new_roots[i], new_values[i], old_root, old_paths[i], old_values[i], old_indicies[i], "update_memberships");

        old_root = new_roots[i];
    }

    return old_root;
}

/**
 * Asserts if old and new state of the tree is correct after a subtree-update.
 *
 * @param new_root: The root of the updated merkle tree,
 * @param new_subtree_root: The new value of the subtree root,
 * @param old_root: The root of the merkle tree before it was updated,
 * @param old_hashes: The hash path from any leaf in the subtree to the root, it doesn't matter if this hash path is
 * computed before or after updating the tree,
 * @param old_subtree_root: The value of the subtree root before it was updated,
 * @param index: The index of any leaf in the subtree,
 * @param at_height: The height of the subtree,
 * @param msg: error message.
 * @tparam Builder: type of builder.
 */
template <typename Builder>
void update_subtree_membership(field_t<Builder> const& new_root,
                               field_t<Builder> const& new_subtree_root,
                               field_t<Builder> const& old_root,
                               hash_path<Builder> const& old_hashes,
                               field_t<Builder> const& old_subtree_root,
                               bit_vector<Builder> const& index,
                               size_t at_height,
                               std::string const& msg = "update_subtree_membership")
{
    // Check that the old_subtree_root, is in the tree given by old_root, at index and at_height.
    assert_check_subtree_membership(
        old_root, old_hashes, old_subtree_root, index, at_height, false, msg + "_old_subtree");

    // Check that the new_subtree_root, is in the tree given by new_root, at index and at_height.
    // By extracting partner hashes from `old_hashes`, we also validate both membership proofs use
    // identical merkle trees (apart from the leaf that is being updated)
    assert_check_subtree_membership(
        new_root, old_hashes, new_subtree_root, index, at_height, true, msg + "_new_subtree");
}

/**
 * Computes the root of a tree with leaves given as the vector `input`.
 *
 * @param input: vector of leaf values.
 * @tparam Builder: type of builder.
 */
template <typename Builder> field_t<Builder> compute_tree_root(std::vector<field_t<Builder>> const& input)
{
    // Check if the input vector size is a power of 2.
    ASSERT(input.size() > 0);
    ASSERT(!(input.size() & (input.size() - 1)) == true);
    auto layer = input;
    while (layer.size() > 1) {
        std::vector<field_t<Builder>> next_layer(layer.size() / 2);
        for (size_t i = 0; i < next_layer.size(); ++i) {
            next_layer[i] = bb::stdlib::pedersen_hash<Builder>::hash({ layer[i * 2], layer[i * 2 + 1] });
        }
        layer = std::move(next_layer);
    }

    return layer[0];
}

/**
 * Checks if a given root matches the root computed from a set of leaves.
 *
 * @param values: vector of leaf values.
 * @tparam Builder: type of builder.
 */
template <typename Builder>
bool_t<Builder> check_tree(field_t<Builder> const& root, std::vector<field_t<Builder>> const& values)
{
    return compute_tree_root(values) == root;
}

/**
 * Asserts if a given root matches the root computed from a set of leaves.
 *
 * @param values: vector of leaf values.
 * @tparam Builder: type of builder.
 */
template <typename Builder>
void assert_check_tree(field_t<Builder> const& root, std::vector<field_t<Builder>> const& values)
{
    auto valid = check_tree(root, values);
    valid.assert_equal(true, "assert_check_tree");
}

/**
 * Updates the tree with a vector of new values starting from the leaf at start_index.
 *
 * @param new_root: The root of the updated merkle tree,
 * @param old_root: The root of the merkle tree before it was updated,
 * @param old_hashes: The hash path from the leaf at start_index, computed before the tree was updated
 * @param new_values: The vector of values to be inserted from start_index,
 * @param start_index: The index of any leaf from which new values are inserted,
 * @param msg: error message.
 * @tparam Builder: type of builder.
 */
template <typename Builder>
void batch_update_membership(field_t<Builder> const& new_root,
                             field_t<Builder> const& old_root,
                             hash_path<Builder> const& old_path,
                             std::vector<field_t<Builder>> const& new_values,
                             field_t<Builder> const& start_index,
                             std::string const& msg = "batch_update_membership")
{
    size_t height = numeric::get_msb(new_values.size());
    auto zero_subtree_root = field_t<Builder>(zero_hash_at_height(height));

    auto rollup_root = compute_tree_root(new_values);

    update_subtree_membership(
        new_root, rollup_root, old_root, old_path, zero_subtree_root, start_index.decompose_into_bits(), height, msg);
}

} // namespace bb::crypto::merkle_tree

namespace bb::stdlib {
/**
 * @brief Generate a simple merkle tree membership circuit for testing purposes
 *
 * @tparam Builder
 * @param builder
 * @param num_iterations number of membership checks to perform
 */
template <typename Builder> static void generate_merkle_membership_test_circuit(Builder& builder, size_t num_iterations)
{
    using namespace stdlib;
    using field_ct = field_t<Builder>;
    using witness_ct = witness_t<Builder>;
    using MemStore = crypto::merkle_tree::MemoryStore;
    using MerkleTree_ct = crypto::merkle_tree::MerkleTree<MemStore, crypto::merkle_tree::PedersenHashPolicy>;

    MemStore store;
    const size_t tree_depth = 7;
    auto merkle_tree = MerkleTree_ct(store, tree_depth);

    for (size_t i = 0; i < num_iterations; i++) {
        // For each iteration update and check the membership of a different value
        size_t idx = i;
        size_t value = i * 2;
        merkle_tree.update_element(idx, value);

        field_ct root_ct = witness_ct(&builder, merkle_tree.root());
        auto idx_ct = field_ct(witness_ct(&builder, fr(idx))).decompose_into_bits();
        auto value_ct = field_ct(value);

        crypto::merkle_tree::check_membership(
            root_ct,
            crypto::merkle_tree::create_witness_hash_path(builder, merkle_tree.get_hash_path(idx)),
            value_ct,
            idx_ct);
    }
}
} // namespace bb::stdlib