/*
 * MultiVersionTransaction.h
 *
 * This source file is part of the FoundationDB open source project
 *
 * Copyright 2013-2018 Apple Inc. and the FoundationDB project authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FDBCLIENT_MULTIVERSIONTRANSACTION_H
#define FDBCLIENT_MULTIVERSIONTRANSACTION_H
#pragma once

#include "bindings/c/foundationdb/fdb_c_options.g.h"
#include "fdbclient/FDBOptions.g.h"
#include "fdbclient/FDBTypes.h"
#include "fdbclient/IClientApi.h"

#include "flow/ThreadHelper.actor.h"

// FdbCApi is used as a wrapper around the FoundationDB C API that gets loaded from an external client library.
// All of the required functions loaded from that external library are stored in function pointers in this struct.
struct FdbCApi : public ThreadSafeReferenceCounted<FdbCApi> {
	typedef struct FDB_future FDBFuture;
	typedef struct FDB_cluster FDBCluster;
	typedef struct FDB_database FDBDatabase;
	typedef struct FDB_transaction FDBTransaction;

#pragma pack(push, 4)
	typedef struct key {
		const uint8_t* key;
		int keyLength;
	} FDBKey;
	typedef struct keyvalue {
		const void* key;
		int keyLength;
		const void* value;
		int valueLength;
	} FDBKeyValue;
#pragma pack(pop)

	typedef int fdb_error_t;
	typedef int fdb_bool_t;

	typedef void (*FDBCallback)(FDBFuture* future, void* callback_parameter);

	/**
	 * Must be called before any other method.
	 */
	void init(const std::string& fdbCPath, int headerVersion, bool unlinkOnLoad);

	// [[nodiscard]] wrappers around the function pointers. Motivated by a
	// difficult bug caused by ignoring an fdb_error_t in the multi-version
	// client.

	// Network
	[[nodiscard]] fdb_error_t selectApiVersion(int runtimeVersion, int headerVersion) {
		return selectApiVersion_(runtimeVersion, headerVersion);
	}
	[[nodiscard]] const char* getClientVersion() {
		if (!getClientVersion_) {
			return "unknown";
		}
		return getClientVersion_();
	}

	[[nodiscard]] fdb_error_t setNetworkOption(FDBNetworkOption option, uint8_t const* value, int valueLength) {
		return setNetworkOption_(option, value, valueLength);
	}
	[[nodiscard]] fdb_error_t setupNetwork() { return setupNetwork_(); }
	[[nodiscard]] fdb_error_t runNetwork() { return runNetwork_(); }
	[[nodiscard]] fdb_error_t stopNetwork() { return stopNetwork_(); }
	[[nodiscard]] fdb_error_t createDatabase(const char* clusterFilePath, FDBDatabase** db) {
		return createDatabase_(clusterFilePath, db);
	}

	// Database
	[[nodiscard]] fdb_error_t databaseCreateTransaction(FDBDatabase* database, FDBTransaction** tr) {
		return databaseCreateTransaction_(database, tr);
	}
	[[nodiscard]] fdb_error_t databaseSetOption(FDBDatabase* database,
	                                            FDBDatabaseOption option,
	                                            uint8_t const* value,
	                                            int valueLength) {
		return databaseSetOption_(database, option, value, valueLength);
	}
	void databaseDestroy(FDBDatabase* database) { return databaseDestroy_(database); }
	[[nodiscard]] FDBFuture* databaseRebootWorker(FDBDatabase* database,
	                                              uint8_t const* address,
	                                              int addressLength,
	                                              fdb_bool_t check,
	                                              int duration) {

		if (!databaseRebootWorker_) {
			throw unsupported_operation();
		}
		return databaseRebootWorker_(database, address, addressLength, check, duration);
	}
	[[nodiscard]] FDBFuture* databaseForceRecoveryWithDataLoss(FDBDatabase* database,
	                                                           uint8_t const* dcid,
	                                                           int dcidLength) {
		if (!databaseForceRecoveryWithDataLoss_) {
			throw unsupported_operation();
		}

		return databaseForceRecoveryWithDataLoss_(database, dcid, dcidLength);
	}
	[[nodiscard]] FDBFuture* databaseCreateSnapshot(FDBDatabase* database,
	                                                uint8_t const* uid,
	                                                int uidLength,
	                                                uint8_t const* snapshotCommmand,
	                                                int snapshotCommandLength) {
		if (!databaseCreateSnapshot_) {
			throw unsupported_operation();
		}

		return databaseCreateSnapshot_(database, uid, uidLength, snapshotCommmand, snapshotCommandLength);
	}
	[[nodiscard]] double databaseGetMainThreadBusyness(FDBDatabase* database) {
		if (databaseGetMainThreadBusyness_ == nullptr) {
			return 0;
		}

		return databaseGetMainThreadBusyness_(database);
	}
	[[nodiscard]] FDBFuture* databaseGetServerProtocol(FDBDatabase* database, uint64_t expectedVersion) {

		ASSERT(databaseGetServerProtocol_ != nullptr);
		return databaseGetServerProtocol_(database, expectedVersion);
	}

	// Transaction
	[[nodiscard]] fdb_error_t transactionSetOption(FDBTransaction* tr,
	                                               FDBTransactionOption option,
	                                               uint8_t const* value,
	                                               int valueLength) {
		return transactionSetOption_(tr, option, value, valueLength);
	}
	void transactionDestroy(FDBTransaction* tr) { return transactionDestroy_(tr); }

	void transactionSetReadVersion(FDBTransaction* tr, int64_t version) {
		return transactionSetReadVersion_(tr, version);
	}
	[[nodiscard]] FDBFuture* transactionGetReadVersion(FDBTransaction* tr) { return transactionGetReadVersion_(tr); }

	[[nodiscard]] FDBFuture* transactionGet(FDBTransaction* tr,
	                                        uint8_t const* keyName,
	                                        int keyNameLength,
	                                        fdb_bool_t snapshot) {
		return transactionGet_(tr, keyName, keyNameLength, snapshot);
	}
	[[nodiscard]] FDBFuture* transactionGetKey(FDBTransaction* tr,
	                                           uint8_t const* keyName,
	                                           int keyNameLength,
	                                           fdb_bool_t orEqual,
	                                           int offset,
	                                           fdb_bool_t snapshot) {
		return transactionGetKey_(tr, keyName, keyNameLength, orEqual, offset, snapshot);
	}
	[[nodiscard]] FDBFuture* transactionGetAddressesForKey(FDBTransaction* tr,
	                                                       uint8_t const* keyName,
	                                                       int keyNameLength) {
		return transactionGetAddressesForKey_(tr, keyName, keyNameLength);
	}
	[[nodiscard]] FDBFuture* transactionGetRange(FDBTransaction* tr,
	                                             uint8_t const* beginKeyName,
	                                             int beginKeyNameLength,
	                                             fdb_bool_t beginOrEqual,
	                                             int beginOffset,
	                                             uint8_t const* endKeyName,
	                                             int endKeyNameLength,
	                                             fdb_bool_t endOrEqual,
	                                             int endOffset,
	                                             int limit,
	                                             int targetBytes,
	                                             FDBStreamingMode mode,
	                                             int iteration,
	                                             fdb_bool_t snapshot,
	                                             fdb_bool_t reverse) {
		return transactionGetRange_(tr,
		                            beginKeyName,
		                            beginKeyNameLength,
		                            beginOrEqual,
		                            beginOffset,
		                            endKeyName,
		                            endKeyNameLength,
		                            endOrEqual,
		                            endOffset,
		                            limit,
		                            targetBytes,
		                            mode,
		                            iteration,
		                            snapshot,
		                            reverse);
	}
	[[nodiscard]] FDBFuture* transactionGetVersionstamp(FDBTransaction* tr) {

		if (!transactionGetVersionstamp_) {
			throw unsupported_operation();
		}

		return transactionGetVersionstamp_(tr);
	}

	void transactionSet(FDBTransaction* tr,
	                    uint8_t const* keyName,
	                    int keyNameLength,
	                    uint8_t const* value,
	                    int valueLength) {
		return transactionSet_(tr, keyName, keyNameLength, value, valueLength);
	}
	void transactionClear(FDBTransaction* tr, uint8_t const* keyName, int keyNameLength) {
		return transactionClear_(tr, keyName, keyNameLength);
	}
	void transactionClearRange(FDBTransaction* tr,
	                           uint8_t const* beginKeyName,
	                           int beginKeyNameLength,
	                           uint8_t const* endKeyName,
	                           int endKeyNameLength) {
		return transactionClearRange_(tr, beginKeyName, beginKeyNameLength, endKeyName, endKeyNameLength);
	}
	void transactionAtomicOp(FDBTransaction* tr,
	                         uint8_t const* keyName,
	                         int keyNameLength,
	                         uint8_t const* param,
	                         int paramLength,
	                         FDBMutationType operationType) {
		return transactionAtomicOp_(tr, keyName, keyNameLength, param, paramLength, operationType);
	}

	[[nodiscard]] FDBFuture* transactionGetEstimatedRangeSizeBytes(FDBTransaction* tr,
	                                                               uint8_t const* begin_key_name,
	                                                               int begin_key_name_length,
	                                                               uint8_t const* end_key_name,
	                                                               int end_key_name_length) {
		if (!transactionGetEstimatedRangeSizeBytes_) {
			throw unsupported_operation();
		}
		return transactionGetEstimatedRangeSizeBytes_(
		    tr, begin_key_name, begin_key_name_length, end_key_name, end_key_name_length);
	}

	[[nodiscard]] FDBFuture* transactionGetRangeSplitPoints(FDBTransaction* tr,
	                                                        uint8_t const* begin_key_name,
	                                                        int begin_key_name_length,
	                                                        uint8_t const* end_key_name,
	                                                        int end_key_name_length,
	                                                        int64_t chunkSize) {

		if (!transactionGetRangeSplitPoints_) {
			throw unsupported_operation();
		}
		return transactionGetRangeSplitPoints_(
		    tr, begin_key_name, begin_key_name_length, end_key_name, end_key_name_length, chunkSize);
	}

	[[nodiscard]] FDBFuture* transactionCommit(FDBTransaction* tr) { return transactionCommit_(tr); }
	[[nodiscard]] fdb_error_t transactionGetCommittedVersion(FDBTransaction* tr, int64_t* outVersion) {
		return transactionGetCommittedVersion_(tr, outVersion);
	}
	[[nodiscard]] FDBFuture* transactionGetApproximateSize(FDBTransaction* tr) {

		if (!transactionGetApproximateSize_) {
			throw unsupported_operation();
		}

		return transactionGetApproximateSize_(tr);
	}
	[[nodiscard]] FDBFuture* transactionWatch(FDBTransaction* tr, uint8_t const* keyName, int keyNameLength) {
		return transactionWatch_(tr, keyName, keyNameLength);
	}
	[[nodiscard]] FDBFuture* transactionOnError(FDBTransaction* tr, fdb_error_t error) {
		return transactionOnError_(tr, error);
	}
	void transactionReset(FDBTransaction* tr) { return transactionReset_(tr); }
	void transactionCancel(FDBTransaction* tr) { return transactionCancel_(tr); }

	[[nodiscard]] fdb_error_t transactionAddConflictRange(FDBTransaction* tr,
	                                                      uint8_t const* beginKeyName,
	                                                      int beginKeyNameLength,
	                                                      uint8_t const* endKeyName,
	                                                      int endKeyNameLength,
	                                                      FDBConflictRangeType type) {
		return transactionAddConflictRange_(tr, beginKeyName, beginKeyNameLength, endKeyName, endKeyNameLength, type);
	}

	// Future
	[[nodiscard]] fdb_error_t futureGetDatabase(FDBFuture* f, FDBDatabase** outDb) {
		return futureGetDatabase_(f, outDb);
	}
	[[nodiscard]] fdb_error_t futureGetInt64(FDBFuture* f, int64_t* outValue) { return futureGetInt64_(f, outValue); }
	[[nodiscard]] fdb_error_t futureGetUInt64(FDBFuture* f, uint64_t* outValue) {
		return futureGetUInt64_(f, outValue);
	}
	[[nodiscard]] fdb_error_t futureGetBool(FDBFuture* f, bool* outValue) { return futureGetBool_(f, outValue); }
	[[nodiscard]] fdb_error_t futureGetError(FDBFuture* f) { return futureGetError_(f); }
	[[nodiscard]] fdb_error_t futureGetKey(FDBFuture* f, uint8_t const** outKey, int* outKeyLength) {
		return futureGetKey_(f, outKey, outKeyLength);
	}
	[[nodiscard]] fdb_error_t futureGetValue(FDBFuture* f,
	                                         fdb_bool_t* outPresent,
	                                         uint8_t const** outValue,
	                                         int* outValueLength) {
		return futureGetValue_(f, outPresent, outValue, outValueLength);
	}
	[[nodiscard]] fdb_error_t futureGetStringArray(FDBFuture* f, const char*** outStrings, int* outCount) {
		return futureGetStringArray_(f, outStrings, outCount);
	}
	[[nodiscard]] fdb_error_t futureGetKeyArray(FDBFuture* f, FDBKey const** outKeys, int* outCount) {
		return futureGetKeyArray_(f, outKeys, outCount);
	}
	[[nodiscard]] fdb_error_t futureGetKeyValueArray(FDBFuture* f,
	                                                 FDBKeyValue const** outKV,
	                                                 int* outCount,
	                                                 fdb_bool_t* outMore) {
		return futureGetKeyValueArray_(f, outKV, outCount, outMore);
	}
	[[nodiscard]] fdb_error_t futureSetCallback(FDBFuture* f, FDBCallback callback, void* callback_parameter) {
		return futureSetCallback_(f, callback, callback_parameter);
	}
	void futureCancel(FDBFuture* f) { return futureCancel_(f); }
	void futureDestroy(FDBFuture* f) { return futureDestroy_(f); }

	// Legacy Support
	[[nodiscard]] FDBFuture* createCluster(const char* clusterFilePath) { return createCluster_(clusterFilePath); }
	[[nodiscard]] FDBFuture* clusterCreateDatabase(FDBCluster* cluster, uint8_t* dbName, int dbNameLength) {
		return clusterCreateDatabase_(cluster, dbName, dbNameLength);
	}
	void clusterDestroy(FDBCluster* cluster) { return clusterDestroy_(cluster); }
	[[nodiscard]] fdb_error_t futureGetCluster(FDBFuture* f, FDBCluster** outCluster) {
		return futureGetCluster_(f, outCluster);
	}

private:
	friend class DLTest;

	// Network
	fdb_error_t (*selectApiVersion_)(int runtimeVersion, int headerVersion);
	const char* (*getClientVersion_)();
	fdb_error_t (*setNetworkOption_)(FDBNetworkOption option, uint8_t const* value, int valueLength);
	fdb_error_t (*setupNetwork_)();
	fdb_error_t (*runNetwork_)();
	fdb_error_t (*stopNetwork_)();
	fdb_error_t (*createDatabase_)(const char* clusterFilePath, FDBDatabase** db);

	// Database
	fdb_error_t (*databaseCreateTransaction_)(FDBDatabase* database, FDBTransaction** tr);
	fdb_error_t (*databaseSetOption_)(FDBDatabase* database,
	                                  FDBDatabaseOption option,
	                                  uint8_t const* value,
	                                  int valueLength);
	void (*databaseDestroy_)(FDBDatabase* database);
	FDBFuture* (*databaseRebootWorker_)(FDBDatabase* database,
	                                    uint8_t const* address,
	                                    int addressLength,
	                                    fdb_bool_t check,
	                                    int duration);
	FDBFuture* (*databaseForceRecoveryWithDataLoss_)(FDBDatabase* database, uint8_t const* dcid, int dcidLength);
	FDBFuture* (*databaseCreateSnapshot_)(FDBDatabase* database,
	                                      uint8_t const* uid,
	                                      int uidLength,
	                                      uint8_t const* snapshotCommmand,
	                                      int snapshotCommandLength);
	double (*databaseGetMainThreadBusyness_)(FDBDatabase* database);
	FDBFuture* (*databaseGetServerProtocol_)(FDBDatabase* database, uint64_t expectedVersion);

	// Transaction
	fdb_error_t (*transactionSetOption_)(FDBTransaction* tr,
	                                     FDBTransactionOption option,
	                                     uint8_t const* value,
	                                     int valueLength);
	void (*transactionDestroy_)(FDBTransaction* tr);

	void (*transactionSetReadVersion_)(FDBTransaction* tr, int64_t version);
	FDBFuture* (*transactionGetReadVersion_)(FDBTransaction* tr);

	FDBFuture* (*transactionGet_)(FDBTransaction* tr, uint8_t const* keyName, int keyNameLength, fdb_bool_t snapshot);
	FDBFuture* (*transactionGetKey_)(FDBTransaction* tr,
	                                 uint8_t const* keyName,
	                                 int keyNameLength,
	                                 fdb_bool_t orEqual,
	                                 int offset,
	                                 fdb_bool_t snapshot);
	FDBFuture* (*transactionGetAddressesForKey_)(FDBTransaction* tr, uint8_t const* keyName, int keyNameLength);
	FDBFuture* (*transactionGetRange_)(FDBTransaction* tr,
	                                   uint8_t const* beginKeyName,
	                                   int beginKeyNameLength,
	                                   fdb_bool_t beginOrEqual,
	                                   int beginOffset,
	                                   uint8_t const* endKeyName,
	                                   int endKeyNameLength,
	                                   fdb_bool_t endOrEqual,
	                                   int endOffset,
	                                   int limit,
	                                   int targetBytes,
	                                   FDBStreamingMode mode,
	                                   int iteration,
	                                   fdb_bool_t snapshot,
	                                   fdb_bool_t reverse);
	FDBFuture* (*transactionGetVersionstamp_)(FDBTransaction* tr);

	void (*transactionSet_)(FDBTransaction* tr,
	                        uint8_t const* keyName,
	                        int keyNameLength,
	                        uint8_t const* value,
	                        int valueLength);
	void (*transactionClear_)(FDBTransaction* tr, uint8_t const* keyName, int keyNameLength);
	void (*transactionClearRange_)(FDBTransaction* tr,
	                               uint8_t const* beginKeyName,
	                               int beginKeyNameLength,
	                               uint8_t const* endKeyName,
	                               int endKeyNameLength);
	void (*transactionAtomicOp_)(FDBTransaction* tr,
	                             uint8_t const* keyName,
	                             int keyNameLength,
	                             uint8_t const* param,
	                             int paramLength,
	                             FDBMutationType operationType);

	FDBFuture* (*transactionGetEstimatedRangeSizeBytes_)(FDBTransaction* tr,
	                                                     uint8_t const* begin_key_name,
	                                                     int begin_key_name_length,
	                                                     uint8_t const* end_key_name,
	                                                     int end_key_name_length);

	FDBFuture* (*transactionGetRangeSplitPoints_)(FDBTransaction* tr,
	                                              uint8_t const* begin_key_name,
	                                              int begin_key_name_length,
	                                              uint8_t const* end_key_name,
	                                              int end_key_name_length,
	                                              int64_t chunkSize);

	FDBFuture* (*transactionCommit_)(FDBTransaction* tr);
	fdb_error_t (*transactionGetCommittedVersion_)(FDBTransaction* tr, int64_t* outVersion);
	FDBFuture* (*transactionGetApproximateSize_)(FDBTransaction* tr);
	FDBFuture* (*transactionWatch_)(FDBTransaction* tr, uint8_t const* keyName, int keyNameLength);
	FDBFuture* (*transactionOnError_)(FDBTransaction* tr, fdb_error_t error);
	void (*transactionReset_)(FDBTransaction* tr);
	void (*transactionCancel_)(FDBTransaction* tr);

	fdb_error_t (*transactionAddConflictRange_)(FDBTransaction* tr,
	                                            uint8_t const* beginKeyName,
	                                            int beginKeyNameLength,
	                                            uint8_t const* endKeyName,
	                                            int endKeyNameLength,
	                                            FDBConflictRangeType);

	// Future
	fdb_error_t (*futureGetDatabase_)(FDBFuture* f, FDBDatabase** outDb);
	fdb_error_t (*futureGetInt64_)(FDBFuture* f, int64_t* outValue);
	fdb_error_t (*futureGetUInt64_)(FDBFuture* f, uint64_t* outValue);
	fdb_error_t (*futureGetBool_)(FDBFuture* f, bool* outValue);
	fdb_error_t (*futureGetError_)(FDBFuture* f);
	fdb_error_t (*futureGetKey_)(FDBFuture* f, uint8_t const** outKey, int* outKeyLength);
	fdb_error_t (*futureGetValue_)(FDBFuture* f, fdb_bool_t* outPresent, uint8_t const** outValue, int* outValueLength);
	fdb_error_t (*futureGetStringArray_)(FDBFuture* f, const char*** outStrings, int* outCount);
	fdb_error_t (*futureGetKeyArray_)(FDBFuture* f, FDBKey const** outKeys, int* outCount);
	fdb_error_t (*futureGetKeyValueArray_)(FDBFuture* f, FDBKeyValue const** outKV, int* outCount, fdb_bool_t* outMore);
	fdb_error_t (*futureSetCallback_)(FDBFuture* f, FDBCallback callback, void* callback_parameter);
	void (*futureCancel_)(FDBFuture* f);
	void (*futureDestroy_)(FDBFuture* f);

	// Legacy Support
	FDBFuture* (*createCluster_)(const char* clusterFilePath);
	FDBFuture* (*clusterCreateDatabase_)(FDBCluster* cluster, uint8_t* dbName, int dbNameLength);
	void (*clusterDestroy_)(FDBCluster* cluster);
	fdb_error_t (*futureGetCluster_)(FDBFuture* f, FDBCluster** outCluster);
};

// An implementation of ITransaction that wraps a transaction object created on an externally loaded client library.
// All API calls to that transaction are routed through the external library.
class DLTransaction : public ITransaction, ThreadSafeReferenceCounted<DLTransaction> {
public:
	DLTransaction(Reference<FdbCApi> api, FdbCApi::FDBTransaction* tr) : api(api), tr(tr) {}
	~DLTransaction() override { api->transactionDestroy(tr); }

	void cancel() override;
	void setVersion(Version v) override;
	ThreadFuture<Version> getReadVersion() override;

	ThreadFuture<Optional<Value>> get(const KeyRef& key, bool snapshot = false) override;
	ThreadFuture<Key> getKey(const KeySelectorRef& key, bool snapshot = false) override;
	ThreadFuture<RangeResult> getRange(const KeySelectorRef& begin,
	                                   const KeySelectorRef& end,
	                                   int limit,
	                                   bool snapshot = false,
	                                   bool reverse = false) override;
	ThreadFuture<RangeResult> getRange(const KeySelectorRef& begin,
	                                   const KeySelectorRef& end,
	                                   GetRangeLimits limits,
	                                   bool snapshot = false,
	                                   bool reverse = false) override;
	ThreadFuture<RangeResult> getRange(const KeyRangeRef& keys,
	                                   int limit,
	                                   bool snapshot = false,
	                                   bool reverse = false) override;
	ThreadFuture<RangeResult> getRange(const KeyRangeRef& keys,
	                                   GetRangeLimits limits,
	                                   bool snapshot = false,
	                                   bool reverse = false) override;
	ThreadFuture<Standalone<VectorRef<const char*>>> getAddressesForKey(const KeyRef& key) override;
	ThreadFuture<Standalone<StringRef>> getVersionstamp() override;
	ThreadFuture<int64_t> getEstimatedRangeSizeBytes(const KeyRangeRef& keys) override;
	ThreadFuture<Standalone<VectorRef<KeyRef>>> getRangeSplitPoints(const KeyRangeRef& range,
	                                                                int64_t chunkSize) override;

	void addReadConflictRange(const KeyRangeRef& keys) override;

	void atomicOp(const KeyRef& key, const ValueRef& value, uint32_t operationType) override;
	void set(const KeyRef& key, const ValueRef& value) override;
	void clear(const KeyRef& begin, const KeyRef& end) override;
	void clear(const KeyRangeRef& range) override;
	void clear(const KeyRef& key) override;

	ThreadFuture<Void> watch(const KeyRef& key) override;

	void addWriteConflictRange(const KeyRangeRef& keys) override;

	ThreadFuture<Void> commit() override;
	Version getCommittedVersion() override;
	ThreadFuture<int64_t> getApproximateSize() override;

	void setOption(FDBTransactionOptions::Option option, Optional<StringRef> value = Optional<StringRef>()) override;

	ThreadFuture<Void> onError(Error const& e) override;
	void reset() override;

	void addref() override { ThreadSafeReferenceCounted<DLTransaction>::addref(); }
	void delref() override { ThreadSafeReferenceCounted<DLTransaction>::delref(); }

private:
	const Reference<FdbCApi> api;
	FdbCApi::FDBTransaction* const tr;
};

// An implementation of IDatabase that wraps a database object created on an externally loaded client library.
// All API calls to that database are routed through the external library.
class DLDatabase : public IDatabase, ThreadSafeReferenceCounted<DLDatabase> {
public:
	DLDatabase(Reference<FdbCApi> api, FdbCApi::FDBDatabase* db) : api(api), db(db), ready(Void()) {}
	DLDatabase(Reference<FdbCApi> api, ThreadFuture<FdbCApi::FDBDatabase*> dbFuture);
	~DLDatabase() override {
		if (db) {
			api->databaseDestroy(db);
		}
	}

	ThreadFuture<Void> onReady();

	Reference<ITransaction> createTransaction() override;
	void setOption(FDBDatabaseOptions::Option option, Optional<StringRef> value = Optional<StringRef>()) override;
	double getMainThreadBusyness() override;

	// Returns the protocol version reported by the coordinator this client is connected to
	// If an expected version is given, the future won't return until the protocol version is different than expected
	// Note: this will never return if the server is running a protocol from FDB 5.0 or older
	ThreadFuture<ProtocolVersion> getServerProtocol(
	    Optional<ProtocolVersion> expectedVersion = Optional<ProtocolVersion>()) override;

	void addref() override { ThreadSafeReferenceCounted<DLDatabase>::addref(); }
	void delref() override { ThreadSafeReferenceCounted<DLDatabase>::delref(); }

	ThreadFuture<int64_t> rebootWorker(const StringRef& address, bool check, int duration) override;
	ThreadFuture<Void> forceRecoveryWithDataLoss(const StringRef& dcid) override;
	ThreadFuture<Void> createSnapshot(const StringRef& uid, const StringRef& snapshot_command) override;

private:
	const Reference<FdbCApi> api;
	FdbCApi::FDBDatabase*
	    db; // Always set if API version >= 610, otherwise guaranteed to be set when onReady future is set
	ThreadFuture<Void> ready;
};

// An implementation of IClientApi that re-issues API calls to the C API of an externally loaded client library.
// The DL prefix stands for "dynamic library".
class DLApi : public IClientApi {
public:
	DLApi(std::string fdbCPath, bool unlinkOnLoad = false);

	void selectApiVersion(int apiVersion) override;
	const char* getClientVersion() override;

	void setNetworkOption(FDBNetworkOptions::Option option, Optional<StringRef> value = Optional<StringRef>()) override;
	void setupNetwork() override;
	void runNetwork() override;
	void stopNetwork() override;

	Reference<IDatabase> createDatabase(const char* clusterFilePath) override;
	Reference<IDatabase> createDatabase609(const char* clusterFilePath); // legacy database creation

	void addNetworkThreadCompletionHook(void (*hook)(void*), void* hookParameter) override;

private:
	const std::string fdbCPath;
	const Reference<FdbCApi> api;
	const bool unlinkOnLoad;
	int headerVersion;
	bool networkSetup;

	Mutex lock;
	std::vector<std::pair<void (*)(void*), void*>> threadCompletionHooks;
};

class MultiVersionDatabase;

// An implementation of ITransaction that wraps a transaction created either locally or through a dynamically loaded
// external client. When needed (e.g on cluster version change), the MultiVersionTransaction can automatically replace
// its wrapped transaction with one from another client.
class MultiVersionTransaction : public ITransaction, ThreadSafeReferenceCounted<MultiVersionTransaction> {
public:
	MultiVersionTransaction(Reference<MultiVersionDatabase> db,
	                        UniqueOrderedOptionList<FDBTransactionOptions> defaultOptions);

	void cancel() override;
	void setVersion(Version v) override;
	ThreadFuture<Version> getReadVersion() override;

	ThreadFuture<Optional<Value>> get(const KeyRef& key, bool snapshot = false) override;
	ThreadFuture<Key> getKey(const KeySelectorRef& key, bool snapshot = false) override;
	ThreadFuture<RangeResult> getRange(const KeySelectorRef& begin,
	                                   const KeySelectorRef& end,
	                                   int limit,
	                                   bool snapshot = false,
	                                   bool reverse = false) override;
	ThreadFuture<RangeResult> getRange(const KeySelectorRef& begin,
	                                   const KeySelectorRef& end,
	                                   GetRangeLimits limits,
	                                   bool snapshot = false,
	                                   bool reverse = false) override;
	ThreadFuture<RangeResult> getRange(const KeyRangeRef& keys,
	                                   int limit,
	                                   bool snapshot = false,
	                                   bool reverse = false) override;
	ThreadFuture<RangeResult> getRange(const KeyRangeRef& keys,
	                                   GetRangeLimits limits,
	                                   bool snapshot = false,
	                                   bool reverse = false) override;
	ThreadFuture<Standalone<VectorRef<const char*>>> getAddressesForKey(const KeyRef& key) override;
	ThreadFuture<Standalone<StringRef>> getVersionstamp() override;

	void addReadConflictRange(const KeyRangeRef& keys) override;
	ThreadFuture<int64_t> getEstimatedRangeSizeBytes(const KeyRangeRef& keys) override;
	ThreadFuture<Standalone<VectorRef<KeyRef>>> getRangeSplitPoints(const KeyRangeRef& range,
	                                                                int64_t chunkSize) override;

	void atomicOp(const KeyRef& key, const ValueRef& value, uint32_t operationType) override;
	void set(const KeyRef& key, const ValueRef& value) override;
	void clear(const KeyRef& begin, const KeyRef& end) override;
	void clear(const KeyRangeRef& range) override;
	void clear(const KeyRef& key) override;

	ThreadFuture<Void> watch(const KeyRef& key) override;

	void addWriteConflictRange(const KeyRangeRef& keys) override;

	ThreadFuture<Void> commit() override;
	Version getCommittedVersion() override;
	ThreadFuture<int64_t> getApproximateSize() override;

	void setOption(FDBTransactionOptions::Option option, Optional<StringRef> value = Optional<StringRef>()) override;

	ThreadFuture<Void> onError(Error const& e) override;
	void reset() override;

	void addref() override { ThreadSafeReferenceCounted<MultiVersionTransaction>::addref(); }
	void delref() override { ThreadSafeReferenceCounted<MultiVersionTransaction>::delref(); }

private:
	const Reference<MultiVersionDatabase> db;
	ThreadSpinLock lock;

	struct TransactionInfo {
		Reference<ITransaction> transaction;
		ThreadFuture<Void> onChange;
	};

	TransactionInfo transaction;

	TransactionInfo getTransaction();
	void updateTransaction();
	void setDefaultOptions(UniqueOrderedOptionList<FDBTransactionOptions> options);

	std::vector<std::pair<FDBTransactionOptions::Option, Optional<Standalone<StringRef>>>> persistentOptions;
};

struct ClientDesc {
	std::string const libPath;
	bool const external;

	ClientDesc(std::string libPath, bool external) : libPath(libPath), external(external) {}
};

struct ClientInfo : ClientDesc, ThreadSafeReferenceCounted<ClientInfo> {
	ProtocolVersion protocolVersion;
	IClientApi* api;
	bool failed;
	std::vector<std::pair<void (*)(void*), void*>> threadCompletionHooks;

	ClientInfo() : ClientDesc(std::string(), false), protocolVersion(0), api(nullptr), failed(true) {}
	ClientInfo(IClientApi* api) : ClientDesc("internal", false), protocolVersion(0), api(api), failed(false) {}
	ClientInfo(IClientApi* api, std::string libPath)
	  : ClientDesc(libPath, true), protocolVersion(0), api(api), failed(false) {}

	void loadProtocolVersion();
	bool canReplace(Reference<ClientInfo> other) const;
};

class MultiVersionApi;

// An implementation of IDatabase that wraps a database created either locally or through a dynamically loaded
// external client. The MultiVersionDatabase monitors the protocol version of the cluster and automatically
// replaces the wrapped database when the protocol version changes.
class MultiVersionDatabase final : public IDatabase, ThreadSafeReferenceCounted<MultiVersionDatabase> {
public:
	MultiVersionDatabase(MultiVersionApi* api,
	                     int threadIdx,
	                     std::string clusterFilePath,
	                     Reference<IDatabase> db,
	                     Reference<IDatabase> versionMonitorDb,
	                     bool openConnectors = true);

	~MultiVersionDatabase() override;

	Reference<ITransaction> createTransaction() override;
	void setOption(FDBDatabaseOptions::Option option, Optional<StringRef> value = Optional<StringRef>()) override;
	double getMainThreadBusyness() override;

	// Returns the protocol version reported by the coordinator this client is connected to
	// If an expected version is given, the future won't return until the protocol version is different than expected
	// Note: this will never return if the server is running a protocol from FDB 5.0 or older
	ThreadFuture<ProtocolVersion> getServerProtocol(
	    Optional<ProtocolVersion> expectedVersion = Optional<ProtocolVersion>()) override;

	void addref() override { ThreadSafeReferenceCounted<MultiVersionDatabase>::addref(); }
	void delref() override { ThreadSafeReferenceCounted<MultiVersionDatabase>::delref(); }

	// Create a MultiVersionDatabase that wraps an already created IDatabase object
	// For internal use in testing
	static Reference<IDatabase> debugCreateFromExistingDatabase(Reference<IDatabase> db);

	ThreadFuture<int64_t> rebootWorker(const StringRef& address, bool check, int duration) override;
	ThreadFuture<Void> forceRecoveryWithDataLoss(const StringRef& dcid) override;
	ThreadFuture<Void> createSnapshot(const StringRef& uid, const StringRef& snapshot_command) override;

	// private:

	struct LegacyVersionMonitor;

	// A struct that manages the current connection state of the MultiVersionDatabase. This wraps the underlying
	// IDatabase object that is currently interacting with the cluster.
	struct DatabaseState : ThreadSafeReferenceCounted<DatabaseState> {
		DatabaseState(std::string clusterFilePath, Reference<IDatabase> versionMonitorDb);

		// Replaces the active database connection with a new one. Must be called from the main thread.
		void updateDatabase(Reference<IDatabase> newDb, Reference<ClientInfo> client);

		// Called when a change to the protocol version of the cluster has been detected.
		// Must be called from the main thread
		void protocolVersionChanged(ProtocolVersion protocolVersion);

		// Adds a client (local or externally loaded) that can be used to connect to the cluster
		void addClient(Reference<ClientInfo> client);

		// Watch the cluster protocol version for changes and update the database state when it does.
		// Must be called from the main thread
		ThreadFuture<Void> monitorProtocolVersion();

		// Starts version monitors for old client versions that don't support connect packet monitoring (<= 5.0).
		// Must be called from the main thread
		void startLegacyVersionMonitors();

		// Cleans up state for the legacy version monitors to break reference cycles
		void close();

		Reference<IDatabase> db;
		const Reference<ThreadSafeAsyncVar<Reference<IDatabase>>> dbVar;
		std::string clusterFilePath;

		// Used to monitor the cluster protocol version. Will be the same as db unless we have either not connected
		// yet or if the client version associated with db does not support protocol monitoring. In those cases,
		// this will be a specially created local db.
		Reference<IDatabase> versionMonitorDb;

		ThreadFuture<Void> changed;

		bool cancelled;

		ThreadFuture<Void> dbReady;
		ThreadFuture<Void> protocolVersionMonitor;

		// Versions older than 6.1 do not benefit from having their database connections closed. Additionally,
		// there are various issues that result in negative behavior in some cases if the connections are closed.
		// Therefore, we leave them open.
		std::map<ProtocolVersion, Reference<IDatabase>> legacyDatabaseConnections;

		// Versions 5.0 and older do not support connection packet monitoring and require alternate techniques to
		// determine the cluster version.
		std::list<Reference<LegacyVersionMonitor>> legacyVersionMonitors;

		Optional<ProtocolVersion> dbProtocolVersion;

		// This maps a normalized protocol version to the client associated with it. This prevents compatible
		// differences in protocol version not matching each other.
		std::map<ProtocolVersion, Reference<ClientInfo>> clients;

		std::vector<std::pair<FDBDatabaseOptions::Option, Optional<Standalone<StringRef>>>> options;
		UniqueOrderedOptionList<FDBTransactionOptions> transactionDefaultOptions;
		Mutex optionLock;
	};

	// A struct that enables monitoring whether the cluster is running an old version (<= 5.0) that doesn't support
	// connect packet monitoring.
	struct LegacyVersionMonitor : ThreadSafeReferenceCounted<LegacyVersionMonitor> {
		LegacyVersionMonitor(Reference<ClientInfo> const& client) : client(client), monitorRunning(false) {}

		// Terminates the version monitor to break reference cycles
		void close();

		// Starts the connection monitor by creating a database object at an old version.
		// Must be called from the main thread
		void startConnectionMonitor(Reference<DatabaseState> dbState);

		// Runs a GRV probe on the cluster to determine if the client version is compatible with the cluster.
		// Must be called from main thread
		void runGrvProbe(Reference<DatabaseState> dbState);

		Reference<ClientInfo> client;
		Reference<IDatabase> db;
		Reference<ITransaction> tr;

		ThreadFuture<Void> versionMonitor;
		bool monitorRunning;
	};

	const Reference<DatabaseState> dbState;
	friend class MultiVersionTransaction;

	// Clients must create a database object in order to initialize some of their state.
	// This needs to be done only once, and this flag tracks whether that has happened.
	static std::atomic_flag externalClientsInitialized;
};

// An implementation of IClientApi that can choose between multiple different client implementations either provided
// locally within the primary loaded fdb_c client or through any number of dynamically loaded clients.
//
// This functionality is used to provide support for multiple protocol versions simultaneously.
class MultiVersionApi : public IClientApi {
public:
	void selectApiVersion(int apiVersion) override;
	const char* getClientVersion() override;

	void setNetworkOption(FDBNetworkOptions::Option option, Optional<StringRef> value = Optional<StringRef>()) override;
	void setupNetwork() override;
	void runNetwork() override;
	void stopNetwork() override;
	void addNetworkThreadCompletionHook(void (*hook)(void*), void* hookParameter) override;

	// Creates an IDatabase object that represents a connection to the cluster
	Reference<IDatabase> createDatabase(const char* clusterFilePath) override;
	static MultiVersionApi* api;

	Reference<ClientInfo> getLocalClient();
	void runOnExternalClients(int threadId,
	                          std::function<void(Reference<ClientInfo>)>,
	                          bool runOnFailedClients = false);
	void runOnExternalClientsAllThreads(std::function<void(Reference<ClientInfo>)>, bool runOnFailedClients = false);

	void updateSupportedVersions();

	bool callbackOnMainThread;
	bool localClientDisabled;

	static bool apiVersionAtLeast(int minVersion);

private:
	MultiVersionApi();

	void loadEnvironmentVariableNetworkOptions();

	void disableMultiVersionClientApi();
	void setCallbacksOnExternalThreads();
	void addExternalLibrary(std::string path);
	void addExternalLibraryDirectory(std::string path);
	// Return a vector of (pathname, unlink_on_close) pairs.  Makes threadCount - 1 copies of the library stored in
	// path, and returns a vector of length threadCount.
	std::vector<std::pair<std::string, bool>> copyExternalLibraryPerThread(std::string path);
	void disableLocalClient();
	void setSupportedClientVersions(Standalone<StringRef> versions);

	void setNetworkOptionInternal(FDBNetworkOptions::Option option, Optional<StringRef> value);

	Reference<ClientInfo> localClient;
	std::map<std::string, ClientDesc> externalClientDescriptions;
	std::map<std::string, std::vector<Reference<ClientInfo>>> externalClients;

	bool networkStartSetup;
	volatile bool networkSetup;
	volatile bool bypassMultiClientApi;
	volatile bool externalClient;
	int apiVersion;

	int nextThread = 0;
	int threadCount;

	Mutex lock;
	std::vector<std::pair<FDBNetworkOptions::Option, Optional<Standalone<StringRef>>>> options;
	std::map<FDBNetworkOptions::Option, std::set<Standalone<StringRef>>> setEnvOptions;
	volatile bool envOptionsLoaded;
};

#endif
