mod combine_data;

use crate::components::public_tail_output_composer::combine_data::combine_data;
use dep::types::{
    abis::{kernel_circuit_public_inputs::{KernelCircuitPublicInputs, PublicKernelCircuitPublicInputs}},
    partial_state_reference::PartialStateReference
};

struct PublicTailOutputComposer {
    previous_kernel: PublicKernelCircuitPublicInputs,
    start_state: PartialStateReference,
}

impl PublicTailOutputComposer {
    pub fn new(
        previous_kernel: PublicKernelCircuitPublicInputs,
        start_state: PartialStateReference
    ) -> Self {
        PublicTailOutputComposer { previous_kernel, start_state }
    }

    pub fn finish(self) -> KernelCircuitPublicInputs {
        let end = combine_data(
            self.previous_kernel.end_non_revertible,
            self.previous_kernel.end
        );

        KernelCircuitPublicInputs {
            rollup_validation_requests: self.previous_kernel.validation_requests.for_rollup,
            end,
            constants: self.previous_kernel.constants,
            start_state: self.start_state,
            revert_code: self.previous_kernel.revert_code,
            fee_payer: self.previous_kernel.fee_payer
        }
    }
}
