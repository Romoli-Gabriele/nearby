// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef THIRD_PARTY_NEARBY_INTERNAL_WEAVE_BASE_SOCKET_H_
#define THIRD_PARTY_NEARBY_INTERNAL_WEAVE_BASE_SOCKET_H_

#include <atomic>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "absl/base/thread_annotations.h"
#include "internal/platform/mutex.h"
#include "internal/platform/single_thread_executor.h"
#include "internal/weave/connection.h"
#include "internal/weave/control_packet_write_request.h"
#include "internal/weave/message_write_request.h"
#include "internal/weave/packet.h"
#include "internal/weave/packet_sequence_number_generator.h"
#include "internal/weave/packetizer.h"
#include "internal/weave/socket_callback.h"

namespace nearby {
namespace weave {

class BaseSocket {
 public:
  BaseSocket(Connection* connection, SocketCallback&& callback);
  virtual ~BaseSocket();
  bool IsConnected();
  void Disconnect();
  nearby::Future<absl::Status> Write(ByteArray message);
  virtual void Connect() = 0;

 protected:
  SocketCallback socket_callback_;
  void OnConnected(int new_max_packet_size);
  void DisconnectInternal(absl::Status status);
  virtual bool IsConnectingOrConnected();
  virtual void DisconnectQuietly();
  virtual void OnReceiveControlPacket(Packet packet) {}
  virtual void WriteControlPacket(Packet packet);
  void OnReceiveDataPacket(Packet packet);
  Connection* connection_;
  void RunOnSocketThread(std::string name, Runnable&& runnable) {
    NEARBY_LOGS(INFO) << "RunOnSocketThread: " << name;
    executor_.Execute(name, std::move(runnable));
  }
  void ShutDown();

 private:
  bool IsRemotePacketCounterExpected(int counter);
  void TryWriteControl() ABSL_EXCLUSIVE_LOCKS_REQUIRED(executor_)
      ABSL_LOCKS_EXCLUDED(mutex_);
  void TryWriteMessage() ABSL_EXCLUSIVE_LOCKS_REQUIRED(executor_)
      ABSL_LOCKS_EXCLUDED(mutex_);
  void OnWriteResult(absl::Status status) ABSL_LOCKS_EXCLUDED(executor_);
  void WritePacket(absl::StatusOr<Packet> packet);
  Mutex mutex_;
  std::deque<ControlPacketWriteRequest> controls_ ABSL_GUARDED_BY(mutex_) = {};
  std::deque<MessageWriteRequest> messages_ ABSL_GUARDED_BY(mutex_) = {};
  std::optional<std::reference_wrapper<ControlPacketWriteRequest>>
      current_control_;
  std::optional<std::reference_wrapper<MessageWriteRequest>> current_message_;
  std::atomic_bool connected_ = false;
  std::atomic_bool is_disconnecting_ = false;
  int max_packet_size_;
  Packetizer packetizer_ = {};
  PacketSequenceNumberGenerator packet_counter_generator_;
  PacketSequenceNumberGenerator remote_packet_counter_generator_;
  SingleThreadExecutor executor_;
};

}  // namespace weave
}  // namespace nearby

#endif  // THIRD_PARTY_NEARBY_INTERNAL_WEAVE_BASE_SOCKET_H_
