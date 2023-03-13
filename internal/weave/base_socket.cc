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

#include "internal/weave/base_socket.h"

#include <deque>
#include <string>
#include <utility>

#include "absl/base/thread_annotations.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "internal/platform/byte_array.h"
#include "internal/platform/future.h"
#include "internal/platform/logging.h"
#include "internal/platform/mutex.h"
#include "internal/platform/mutex_lock.h"
#include "internal/weave/connection.h"
#include "internal/weave/control_packet_write_request.h"
#include "internal/weave/message_write_request.h"
#include "internal/weave/packet.h"
#include "internal/weave/socket_callback.h"

namespace nearby {
namespace weave {

// BaseSocket implementation
BaseSocket::BaseSocket(Connection* connection, SocketCallback&& callback) {
  connection_ = connection;
  socket_callback_ = std::move(callback);
  connection_->Initialize(
      {.on_transmit_cb =
           [this](absl::Status status) {
             OnWriteResult(status);
             if (!status.ok()) {
               DisconnectInternal(status);
             }
           },
       .on_remote_transmit_cb =
           [this](std::string message) {
             if (message.empty()) {
               DisconnectInternal(absl::InvalidArgumentError("Empty packet!"));
               return;
             }
             absl::StatusOr<Packet> packet{
                 Packet::FromBytes(ByteArray(message))};
             if (!packet.ok()) {
               DisconnectInternal(packet.status());
               return;
             }
             bool isRemotePacketCounterExpected =
                 IsRemotePacketCounterExpected(packet->GetPacketCounter());
             if (packet->IsControlPacket()) {
               if (!isRemotePacketCounterExpected) {
                 // increment the counter by 1, ignoring the result.
                 remote_packet_counter_generator_.Next();
               }
               OnReceiveControlPacket(std::move(*packet));
             } else {
               if (isRemotePacketCounterExpected) {
                 OnReceiveDataPacket(std::move(*packet));
               } else {
                 Disconnect();
               }
             }
           },
       .on_disconnected_cb = [this]() { DisconnectQuietly(); }});
}

BaseSocket::~BaseSocket() {
  NEARBY_LOGS(INFO) << "~BaseSocket";
  ShutDown();
}

void BaseSocket::ShutDown() {
  executor_.Shutdown();
  NEARBY_LOGS(INFO) << "BaseSocket gone.";
}

void BaseSocket::TryWriteControl() {
  MutexLock lock(&mutex_);
  if (!current_control_.has_value()) {
    if (!controls_.empty()) {
      current_control_ = controls_.front();
    }
  }
  if (!current_control_.has_value() && IsConnected()) {
    return;
  } else {
    current_message_.reset();
    messages_.clear();
  }
  WritePacket(current_control_->get().NextPacket(max_packet_size_));
}

void BaseSocket::TryWriteMessage() {
  bool no_controls = false;
  {
    MutexLock lock(&mutex_);
    no_controls = controls_.empty();
  }
  if (current_control_.has_value() || !no_controls) {
    // We should only be writing one packet.
    TryWriteControl();
    return;
  }
  MutexLock lock(&mutex_);
  if (!current_message_.has_value()) {
    if (!messages_.empty()) {
      current_message_ = messages_.front();
    }
  }
  if (!current_message_.has_value() || current_message_->get().IsFinished() ||
      !IsConnected()) {
    return;
  }
  absl::StatusOr<Packet> packet =
      current_message_->get().NextPacket(max_packet_size_);
  WritePacket(std::move(packet));
}

void BaseSocket::WritePacket(absl::StatusOr<Packet> packet) {
  if (packet.ok()) {
    if (!packet->SetPacketCounter(packet_counter_generator_.Next()).ok()) {
      return;
    }
    NEARBY_LOGS(INFO) << "transmitting pkt";
    connection_->Transmit(packet->GetBytes());
  } else {
    NEARBY_LOGS(WARNING) << packet.status();
    return;
  }
}

void BaseSocket::OnWriteResult(absl::Status status) {
  RunOnSocketThread(
      "OnWriteResult", [this, status]() ABSL_EXCLUSIVE_LOCKS_REQUIRED(
                           executor_) ABSL_LOCKS_EXCLUDED(mutex_) mutable {
        {
          MutexLock lock(&mutex_);
          if (current_control_.has_value()) {
            current_control_.reset();
            controls_.pop_front();
          } else if (current_message_.has_value()) {
            NEARBY_LOGS(INFO) << "OnWriteResult current is not null";
            if (current_message_->get().IsFinished()) {
              NEARBY_LOGS(INFO) << "OnWriteResult current finished";
              current_message_->get().SetWriteStatus(status);
              if (!messages_.empty() &&
                  current_message_->get() == messages_.front()) {
                NEARBY_LOGS(INFO) << "remove message";
                messages_.pop_front();
                current_message_.reset();
              }
            }
          }
        }
        TryWriteMessage();
      });
}

bool BaseSocket::IsRemotePacketCounterExpected(int counter) {
  int expectedPacketCounter = remote_packet_counter_generator_.Next();
  if (counter == expectedPacketCounter) {
    return true;
  } else {
    socket_callback_.on_error_cb(absl::DataLossError(absl::StrFormat(
        "expected remote packet counter %d for packet but got %d",
        expectedPacketCounter, counter)));
    return false;
  }
}

void BaseSocket::Disconnect() {
  RunOnSocketThread("Disconnect", [this]() {
    if (!is_disconnecting_) {
      is_disconnecting_ = true;
      NEARBY_LOGS(INFO) << "is_disconnecting_: " << is_disconnecting_;
      WriteControlPacket(Packet::CreateErrorPacket());
      {
        MutexLock lock(&mutex_);
        current_message_.reset();
        messages_.clear();
      }
      DisconnectQuietly();
    }
  });
}

void BaseSocket::DisconnectQuietly() {
  RunOnSocketThread("ResetDisconnectQuietly",
                    [this]() ABSL_EXCLUSIVE_LOCKS_REQUIRED(executor_)
                        ABSL_LOCKS_EXCLUDED(mutex_) {
                          bool wasConnectingOrConnected =
                              IsConnectingOrConnected();
                          connected_ = false;
                          if (wasConnectingOrConnected) {
                            socket_callback_.on_disconnected_cb();
                          }
                          // Dump message and control queue.
                          {
                            MutexLock lock(&mutex_);
                            messages_.clear();
                            controls_.clear();
                            current_control_.reset();
                            current_message_.reset();
                          }
                          packetizer_.Reset();
                          packet_counter_generator_.Reset();
                          remote_packet_counter_generator_.Reset();
                        });
  NEARBY_LOGS(INFO) << "scheduled reset";
}

void BaseSocket::OnReceiveDataPacket(Packet packet) {
  absl::Status packet_status = packetizer_.AddPacket(std::move(packet));
  if (!packet_status.ok()) {
    DisconnectInternal(packet_status);
  } else {
    absl::StatusOr<ByteArray> message = packetizer_.TakeMessage();
    if (message.ok()) {
      socket_callback_.on_receive_cb(message->string_data());
    } else {
      DisconnectInternal(message.status());
    }
  }
}

nearby::Future<absl::Status> BaseSocket::Write(ByteArray message) {
  MessageWriteRequest request = MessageWriteRequest(message.string_data());
  nearby::Future<absl::Status> ret = request.GetWriteStatusFuture();

  RunOnSocketThread("TryWriteMessage",
                    [&, request = std::move(request)]()
                        ABSL_EXCLUSIVE_LOCKS_REQUIRED(executor_) mutable {
                          {
                            MutexLock lock(&mutex_);
                            messages_.push_back(std::move(request));
                          }
                          TryWriteMessage();
                        });
  return ret;
}

void BaseSocket::WriteControlPacket(Packet packet) {
  ControlPacketWriteRequest request =
      ControlPacketWriteRequest(std::move(packet));

  RunOnSocketThread("TryWriteControl",
                    [&, request = std::move(request)]()
                        ABSL_EXCLUSIVE_LOCKS_REQUIRED(executor_) mutable {
                          {
                            MutexLock lock(&mutex_);
                            controls_.push_back(std::move(request));
                          }
                          TryWriteControl();
                        });
  NEARBY_LOGS(INFO) << "Scheduled TryWriteControl";
}

void BaseSocket::DisconnectInternal(absl::Status status) {
  socket_callback_.on_error_cb(status);
  Disconnect();
}

bool BaseSocket::IsConnected() { return connected_; }

bool BaseSocket::IsConnectingOrConnected() { return connected_; }

void BaseSocket::OnConnected(int new_max_packet_size) {
  RunOnSocketThread("TryWriteOnConnected",
                    [this, new_max_packet_size]()
                        ABSL_EXCLUSIVE_LOCKS_REQUIRED(executor_) {
                          max_packet_size_ = new_max_packet_size;
                          bool was_connected = IsConnected();
                          connected_ = true;
                          is_disconnecting_ = false;
                          if (!was_connected) {
                            socket_callback_.on_connected_cb();
                          }
                          TryWriteMessage();
                        });
}

}  // namespace weave
}  // namespace nearby
