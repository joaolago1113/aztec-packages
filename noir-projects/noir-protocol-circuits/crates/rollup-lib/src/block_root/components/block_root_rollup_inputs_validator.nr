use crate::{
    abis::{block_root_rollup_data::BlockRootRollupData, previous_rollup_data::PreviousRollupData},
    merge::utils::validate_consecutive_rollups::validate_consecutive_rollups,
};
use types::{
    abis::{append_only_tree_snapshot::AppendOnlyTreeSnapshot, sponge_blob::SpongeBlob},
    merkle_tree::membership::check_membership,
    proof::traits::Verifiable,
    traits::Hash,
};

pub struct BlockRootRollupInputsValidator<let NUM_ROLLUPS: u32, let N: u32> {
    previous_rollups: [PreviousRollupData; NUM_ROLLUPS],
    allowed_vk_indices: [u32; N],
    data: BlockRootRollupData,
}

impl<let NUM_ROLLUPS: u32, let N: u32> BlockRootRollupInputsValidator<NUM_ROLLUPS, N> {
    pub fn new(
        previous_rollups: [PreviousRollupData; NUM_ROLLUPS],
        allowed_vk_indices: [u32; N],
        data: BlockRootRollupData,
    ) -> Self {
        BlockRootRollupInputsValidator { previous_rollups, data, allowed_vk_indices }
    }

    pub fn validate(self) {
        assert(NUM_ROLLUPS != 0);

        self.validate_root_parity_proof();
        self.validate_rollup_proofs();
        self.validate_first_rollup();

        let previous_archive =
            self.previous_rollups[0].base_or_merge_rollup_public_inputs.constants.last_archive;
        self.validate_previous_block_header(previous_archive);

        if NUM_ROLLUPS == 1 {
            assert_eq(
                self.previous_rollups[0].base_or_merge_rollup_public_inputs.num_txs,
                1,
                "single-tx block root must contain only 1 tx",
            );
        }

        for i in 1..NUM_ROLLUPS {
            validate_consecutive_rollups(
                self.previous_rollups[i - 1].base_or_merge_rollup_public_inputs,
                self.previous_rollups[i].base_or_merge_rollup_public_inputs,
            );
        }
    }

    pub fn validate_without_previous_rollups(
        self,
        is_padding: bool,
        previous_archive: AppendOnlyTreeSnapshot,
    ) {
        assert(NUM_ROLLUPS == 0);

        if !is_padding {
            self.validate_root_parity_proof();
            self.validate_previous_block_header(previous_archive);
        }
    }

    fn validate_root_parity_proof(self) {
        if !dep::std::runtime::is_unconstrained() {
            self.data.l1_to_l2_roots.verify();
            self.data.l1_to_l2_roots.validate_in_vk_tree();
        }
    }

    fn validate_rollup_proofs(self) {
        if !dep::std::runtime::is_unconstrained() {
            for i in 0..NUM_ROLLUPS {
                self.previous_rollups[i].verify();
                self.previous_rollups[i].validate_in_vk_tree(self.allowed_vk_indices);
            }
        }
    }

    fn validate_first_rollup(self) {
        let first_rollup = self.previous_rollups[0].base_or_merge_rollup_public_inputs;

        assert_eq(
            first_rollup.start,
            self.data.previous_block_header.state.partial,
            "the start state of the block's first tx does not match the state in previous header",
        );

        // Asserts that the first sponge blob was empty to begin with.
        // This prevents injecting unchecked tx effects in the first base of a rollup.
        let expected_sponge_blob = SpongeBlob::new(first_rollup.start_sponge_blob.expected_fields);
        assert(
            first_rollup.start_sponge_blob.eq(expected_sponge_blob),
            "block's first blob sponge was not empty",
        );
    }

    fn validate_previous_block_header(self, previous_archive: AppendOnlyTreeSnapshot) {
        // Makes sure the hash of the previous block header is the last leaf in the previous archive.
        let previous_block_hash = self.data.previous_block_header.hash();
        let last_leaf_index = previous_archive.next_available_leaf_index - 1;

        assert(
            check_membership(
                previous_block_hash,
                last_leaf_index as Field,
                self.data.previous_archive_sibling_path,
                previous_archive.root,
            ),
            "hash of the previous block header is not the last leaf in the archive tree",
        );
    }
}
