import { L2Block, type ViemSignature } from '@aztec/circuit-types';
import { EthAddress } from '@aztec/circuits.js';
import { sleep } from '@aztec/foundation/sleep';
import { NoopTelemetryClient } from '@aztec/telemetry-client/noop';

import { type MockProxy, mock } from 'jest-mock-extended';
import { type GetTransactionReceiptReturnType, type PrivateKeyAccount } from 'viem';

import { type PublisherConfig, type TxSenderConfig } from './config.js';
import { L1Publisher } from './l1-publisher.js';

interface MockAvailabilityOracleWrite {
  publish: (args: readonly [`0x${string}`], options: { account: PrivateKeyAccount }) => Promise<`0x${string}`>;
}

interface MockAvailabilityOracleRead {
  isAvailable: (args: readonly [`0x${string}`]) => Promise<boolean>;
}

class MockAvailabilityOracle {
  constructor(
    public write: MockAvailabilityOracleWrite,
    public simulate: MockAvailabilityOracleWrite,
    public read: MockAvailabilityOracleRead,
  ) {}
}

interface MockPublicClient {
  getTransactionReceipt: ({ hash }: { hash: '0x${string}' }) => Promise<GetTransactionReceiptReturnType>;
  getBlock(): Promise<{ timestamp: bigint }>;
  getTransaction: ({ hash }: { hash: '0x${string}' }) => Promise<{ input: `0x${string}`; hash: `0x${string}` }>;
}

interface MockRollupContractWrite {
  propose: (
    args: readonly [`0x${string}`, `0x${string}`] | readonly [`0x${string}`, `0x${string}`, `0x${string}`],
    options: { account: PrivateKeyAccount },
  ) => Promise<`0x${string}`>;
}

interface MockRollupContractRead {
  archive: () => Promise<`0x${string}`>;
  getCurrentSlot(): Promise<bigint>;
  validateHeader: (
    args: readonly [
      `0x${string}`,
      ViemSignature[],
      `0x${string}`,
      bigint,
      { ignoreDA: boolean; ignoreSignatures: boolean },
    ],
  ) => Promise<void>;
}

class MockRollupContract {
  constructor(
    public write: MockRollupContractWrite,
    public simulate: MockRollupContractWrite,
    public read: MockRollupContractRead,
  ) {}
}

describe('L1Publisher', () => {
  let rollupContractRead: MockProxy<MockRollupContractRead>;
  let rollupContractWrite: MockProxy<MockRollupContractWrite>;
  let rollupContractSimulate: MockProxy<MockRollupContractWrite>;
  let rollupContract: MockRollupContract;

  let availabilityOracleRead: MockProxy<MockAvailabilityOracleRead>;
  let availabilityOracleWrite: MockProxy<MockAvailabilityOracleWrite>;
  let availabilityOracleSimulate: MockProxy<MockAvailabilityOracleWrite>;
  let availabilityOracle: MockAvailabilityOracle;

  let publicClient: MockProxy<MockPublicClient>;

  let processTxHash: `0x${string}`;
  let proposeTxHash: `0x${string}`;
  let processTxReceipt: GetTransactionReceiptReturnType;
  let proposeTxReceipt: GetTransactionReceiptReturnType;
  let l2Block: L2Block;

  let header: Buffer;
  let archive: Buffer;
  let blockHash: Buffer;
  let body: Buffer;

  let account: PrivateKeyAccount;

  let publisher: L1Publisher;

  beforeEach(() => {
    l2Block = L2Block.random(42);

    header = l2Block.header.toBuffer();
    archive = l2Block.archive.root.toBuffer();
    blockHash = l2Block.header.hash().toBuffer();
    body = l2Block.body.toBuffer();

    processTxHash = `0x${Buffer.from('txHashProcess').toString('hex')}`; // random tx hash
    proposeTxHash = `0x${Buffer.from('txHashpropose').toString('hex')}`; // random tx hash

    processTxReceipt = {
      transactionHash: processTxHash,
      status: 'success',
      logs: [],
    } as unknown as GetTransactionReceiptReturnType;
    proposeTxReceipt = {
      transactionHash: proposeTxHash,
      status: 'success',
      logs: [],
    } as unknown as GetTransactionReceiptReturnType;

    rollupContractWrite = mock<MockRollupContractWrite>();
    rollupContractSimulate = mock<MockRollupContractWrite>();
    rollupContractRead = mock<MockRollupContractRead>();
    rollupContract = new MockRollupContract(rollupContractWrite, rollupContractSimulate, rollupContractRead);

    availabilityOracleWrite = mock<MockAvailabilityOracleWrite>();
    availabilityOracleRead = mock<MockAvailabilityOracleRead>();
    availabilityOracleSimulate = mock<MockAvailabilityOracleWrite>();
    availabilityOracle = new MockAvailabilityOracle(
      availabilityOracleWrite,
      availabilityOracleSimulate,
      availabilityOracleRead,
    );

    publicClient = mock<MockPublicClient>();

    const config = {
      l1RpcUrl: `http://127.0.0.1:8545`,
      l1ChainId: 1,
      publisherPrivateKey: `0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80`,
      l1Contracts: {
        availabilityOracleAddress: EthAddress.ZERO.toString(),
        rollupAddress: EthAddress.ZERO.toString(),
      },
      l1PublishRetryIntervalMS: 1,
    } as unknown as TxSenderConfig & PublisherConfig;

    publisher = new L1Publisher(config, new NoopTelemetryClient());

    (publisher as any)['availabilityOracleContract'] = availabilityOracle;
    (publisher as any)['rollupContract'] = rollupContract;
    (publisher as any)['publicClient'] = publicClient;

    account = (publisher as any)['account'];

    rollupContractRead.getCurrentSlot.mockResolvedValue(l2Block.header.globalVariables.slotNumber.toBigInt());
    publicClient.getBlock.mockResolvedValue({ timestamp: 12n });
  });

  it('publishes and propose l2 block to l1', async () => {
    rollupContractRead.archive.mockResolvedValue(l2Block.header.lastArchive.root.toString() as `0x${string}`);
    rollupContractWrite.propose.mockResolvedValueOnce(proposeTxHash);
    rollupContractSimulate.propose.mockResolvedValueOnce(proposeTxHash);
    publicClient.getTransactionReceipt.mockResolvedValueOnce(proposeTxReceipt);

    const result = await publisher.processL2Block(l2Block);

    expect(result).toEqual(true);

    const args = [
      `0x${header.toString('hex')}`,
      `0x${archive.toString('hex')}`,
      `0x${blockHash.toString('hex')}`,
      `0x${body.toString('hex')}`,
    ] as const;
    if (!L1Publisher.SKIP_SIMULATION) {
      expect(rollupContractSimulate.propose).toHaveBeenCalledWith(args, { account: account });
    }
    expect(rollupContractWrite.propose).toHaveBeenCalledWith(args, { account: account });
    expect(publicClient.getTransactionReceipt).toHaveBeenCalledWith({ hash: proposeTxHash });
  });

  it('publishes l2 block to l1 (already published body)', async () => {
    availabilityOracleRead.isAvailable.mockResolvedValueOnce(true);
    rollupContractRead.archive.mockResolvedValue(l2Block.header.lastArchive.root.toString() as `0x${string}`);
    rollupContractWrite.propose.mockResolvedValueOnce(processTxHash);
    rollupContractSimulate.propose.mockResolvedValueOnce(processTxHash);
    publicClient.getTransactionReceipt.mockResolvedValueOnce(processTxReceipt);

    const result = await publisher.processL2Block(l2Block);

    expect(result).toEqual(true);
    const args = [
      `0x${header.toString('hex')}`,
      `0x${archive.toString('hex')}`,
      `0x${blockHash.toString('hex')}`,
    ] as const;
    if (!L1Publisher.SKIP_SIMULATION) {
      expect(rollupContractSimulate.propose).toHaveBeenCalledWith(args, { account });
    }
    expect(rollupContractWrite.propose).toHaveBeenCalledWith(args, { account });
    expect(publicClient.getTransactionReceipt).toHaveBeenCalledWith({ hash: processTxHash });
  });

  it('does not retry if sending a propose tx fails', async () => {
    availabilityOracleRead.isAvailable.mockResolvedValueOnce(true);
    rollupContractRead.archive.mockResolvedValue(l2Block.header.lastArchive.root.toString() as `0x${string}`);
    rollupContractWrite.propose
      .mockRejectedValueOnce(new Error())
      .mockResolvedValueOnce(processTxHash as `0x${string}`);

    // Note that simulate will be valid both times
    rollupContractSimulate.propose
      .mockResolvedValueOnce(processTxHash as `0x${string}`)
      .mockResolvedValueOnce(processTxHash as `0x${string}`);

    const result = await publisher.processL2Block(l2Block);

    expect(result).toEqual(false);
    expect(rollupContractWrite.propose).toHaveBeenCalledTimes(1);
  });

  it('does not retry if simulating a publish and propose tx fails', async () => {
    rollupContractRead.archive.mockResolvedValue(l2Block.header.lastArchive.root.toString() as `0x${string}`);
    rollupContractRead.validateHeader.mockRejectedValueOnce(new Error('Test error'));

    await expect(publisher.processL2Block(l2Block)).rejects.toThrow();

    expect(rollupContractRead.validateHeader).toHaveBeenCalledTimes(1);
    expect(rollupContractWrite.propose).toHaveBeenCalledTimes(0);
  });

  it('does not retry if sending a publish and propose tx fails', async () => {
    rollupContractRead.archive.mockResolvedValue(l2Block.header.lastArchive.root.toString() as `0x${string}`);
    rollupContractSimulate.propose.mockResolvedValueOnce(proposeTxHash as `0x${string}`);
    rollupContractWrite.propose.mockRejectedValueOnce(new Error());

    const result = await publisher.processL2Block(l2Block);

    expect(result).toEqual(false);
    expect(rollupContractWrite.propose).toHaveBeenCalledTimes(1);
  });

  it('retries if fetching the receipt fails (propose)', async () => {
    availabilityOracleRead.isAvailable.mockResolvedValueOnce(true);
    rollupContractRead.archive.mockResolvedValue(l2Block.header.lastArchive.root.toString() as `0x${string}`);
    rollupContractSimulate.propose.mockResolvedValueOnce(processTxHash);
    rollupContractWrite.propose.mockResolvedValueOnce(processTxHash);
    publicClient.getTransactionReceipt.mockRejectedValueOnce(new Error()).mockResolvedValueOnce(processTxReceipt);

    const result = await publisher.processL2Block(l2Block);

    expect(result).toEqual(true);
    expect(publicClient.getTransactionReceipt).toHaveBeenCalledTimes(2);
  });

  it('retries if fetching the receipt fails (publish propose)', async () => {
    rollupContractRead.archive.mockResolvedValue(l2Block.header.lastArchive.root.toString() as `0x${string}`);
    rollupContractSimulate.propose.mockResolvedValueOnce(proposeTxHash as `0x${string}`);
    rollupContractWrite.propose.mockResolvedValueOnce(proposeTxHash as `0x${string}`);
    publicClient.getTransactionReceipt.mockRejectedValueOnce(new Error()).mockResolvedValueOnce(proposeTxReceipt);

    const result = await publisher.processL2Block(l2Block);

    expect(result).toEqual(true);
    expect(publicClient.getTransactionReceipt).toHaveBeenCalledTimes(2);
  });

  it('returns false if publish and propose tx reverts', async () => {
    rollupContractRead.archive.mockResolvedValue(l2Block.header.lastArchive.root.toString() as `0x${string}`);
    rollupContractWrite.propose.mockResolvedValueOnce(proposeTxHash);
    publicClient.getTransactionReceipt.mockResolvedValueOnce({ ...proposeTxReceipt, status: 'reverted' });

    const result = await publisher.processL2Block(l2Block);

    expect(result).toEqual(false);
  });

  it('returns false if propose tx reverts', async () => {
    availabilityOracleRead.isAvailable.mockResolvedValueOnce(true);
    rollupContractRead.archive.mockResolvedValue(l2Block.header.lastArchive.root.toString() as `0x${string}`);

    publicClient.getTransactionReceipt.mockResolvedValueOnce({ ...processTxReceipt, status: 'reverted' });

    const result = await publisher.processL2Block(l2Block);

    expect(result).toEqual(false);
  });

  it('returns false if sending publish and progress tx is interrupted', async () => {
    rollupContractRead.archive.mockResolvedValue(l2Block.header.lastArchive.root.toString() as `0x${string}`);
    rollupContractWrite.propose.mockImplementationOnce(() => sleep(10, proposeTxHash) as Promise<`0x${string}`>);

    const resultPromise = publisher.processL2Block(l2Block);
    publisher.interrupt();
    const result = await resultPromise;

    expect(result).toEqual(false);
    expect(publicClient.getTransactionReceipt).not.toHaveBeenCalled();
  });

  it('returns false if sending propose tx is interrupted', async () => {
    availabilityOracleRead.isAvailable.mockResolvedValueOnce(true);
    rollupContractRead.archive.mockResolvedValue(l2Block.header.lastArchive.root.toString() as `0x${string}`);
    rollupContractWrite.propose.mockImplementationOnce(() => sleep(10, processTxHash) as Promise<`0x${string}`>);

    const resultPromise = publisher.processL2Block(l2Block);
    publisher.interrupt();
    const result = await resultPromise;

    expect(result).toEqual(false);
    expect(publicClient.getTransactionReceipt).not.toHaveBeenCalled();
  });
});
