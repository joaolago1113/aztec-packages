// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Aztec Labs.
pragma solidity >=0.8.27;

import {ValidatorSelection as RealValidatorSelection} from "@aztec/core/ValidatorSelection.sol";
import {TestConstants} from "./TestConstants.sol";
import {TestERC20} from "@aztec/mock/TestERC20.sol";

contract ValidatorSelection is RealValidatorSelection {
  constructor()
    RealValidatorSelection(
      new TestERC20("test", "TEST", address(this)),
      100e18,
      TestConstants.AZTEC_SLASHING_QUORUM,
      TestConstants.AZTEC_SLASHING_ROUND_SIZE,
      TestConstants.AZTEC_SLOT_DURATION,
      TestConstants.AZTEC_EPOCH_DURATION,
      TestConstants.AZTEC_TARGET_COMMITTEE_SIZE
    )
  {}
}
