#!/bin/bash

: '
This script sets up and runs a native testnet for the Aztec network.

Steps:
1. Parse command-line arguments for custom test script, number of validators, and logging level.
2. Set up the base command for running the native network test, including:
   - Running the specified test script
   - Deploying L1 and L2 contracts
   - Starting the boot node
   - Setting up the Ethereum environment
   - Starting the prover node
   - Starting the PXE
   - Starting the transaction bot
3. Dynamically generate commands for the specified number of validators,
   each with an incrementing port number starting from 8081.
4. Execute the complete command to start the testnet.

Usage:
  ./run_native_testnet.sh [options]

Options:
  see display_help() below
'

# Function to display help message
display_help() {
    echo "Usage: $0 [options]"
    echo
    echo "Options:"
    echo "  -h     Display this help message"
    echo "  -t     Specify the test script file (default: ./test-transfer.sh)"
    echo "  -val   Specify the number of validators (default: 3)"
    echo "  -v     Set logging level to verbose"
    echo "  -vv    Set logging level to debug"
    echo "  -i     Run interleaved (default: false)"
    echo
    echo "Example:"
    echo "  $0 -t ./custom-test.sh -val 5 -v"
}

# Default values
TEST_SCRIPT="./test-transfer.sh"
NUM_VALIDATORS=3
LOG_LEVEL="info"
INTERLEAVED=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -h)
      display_help
      exit 0
      ;;
    -t)
      TEST_SCRIPT="$2"
      shift 2
      ;;
    -val)
      NUM_VALIDATORS="$2"
      shift 2
      ;;
    -v)
      if [[ $LOG_LEVEL == "info" ]]; then
        LOG_LEVEL="verbose"
      elif [[ $LOG_LEVEL == "verbose" ]]; then
        LOG_LEVEL="debug"
      fi
      shift
      ;;
    -i)
      INTERLEAVED=true
      shift
      ;;
    -vv)
      LOG_LEVEL="debug"
      shift
      ;;
    *)
      echo "Invalid option: $1" >&2
      display_help
      exit 1
      ;;
  esac
done

## Set log level for all child commands
export LOG_LEVEL

# Base command
BASE_CMD="INTERLEAVED=$INTERLEAVED ./yarn-project/end-to-end/scripts/native_network_test.sh \
        $TEST_SCRIPT \
        ./deploy-l1-contracts.sh \
        ./deploy-l2-contracts.sh \
        ./boot-node.sh \
        ./ethereum.sh \
        \"./prover-node.sh 8078 false\" \
        ./pxe.sh \
        ./transaction-bot.sh \
        \"./validators.sh $NUM_VALIDATORS\""

# Execute the command
eval $BASE_CMD