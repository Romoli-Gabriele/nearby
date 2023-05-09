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

#ifndef THIRD_PARTY_NEARBY_INTERNAL_TEST_FAKE_WEBRTC_H_
#define THIRD_PARTY_NEARBY_INTERNAL_TEST_FAKE_WEBRTC_H_

#include <memory>

#include "internal/platform/webrtc.h"

namespace nearby {

class FakeWebRtcSignalingMessenger : public WebRtcSignalingMessenger {
 public:
  FakeWebRtcSignalingMessenger();
  FakeWebRtcSignalingMessenger(FakeWebRtcSignalingMessenger&&) = delete;
  FakeWebRtcSignalingMessenger operator=(FakeWebRtcSignalingMessenger&&) =
      delete;
  ~FakeWebRtcSignalingMessenger() override;

  // WebRtcSignalingMessenger:
  bool SendMessage(absl::string_view peer_id,
                   const ByteArray& message) override;
  bool StartReceivingMessages(
      OnSignalingMessageCallback on_message_callback,
      OnSignalingCompleteCallback on_complete_callback) override;
  bool IsValid() const override { return is_valid_; }
  void StopReceivingMessages() override {}

  void SetSendMessageResult(bool send_message_result) {
    send_message_result_ = send_message_result;
  }
  void StartReceivingMessagesResult(bool start_receiving_messages_result) {
    start_receiving_messages_result_ = start_receiving_messages_result;
  }
  void SetIsValid(bool is_valid) { is_valid_ = is_valid; }

 private:
  bool send_message_result_ = true;
  bool start_receiving_messages_result_ = true;
  bool is_valid_ = true;
};

class FakeWebRtcMedium : public WebRtcMedium {
 public:
  explicit FakeWebRtcMedium(CancellationFlag* flag);
  FakeWebRtcMedium(FakeWebRtcMedium&&) = delete;
  FakeWebRtcMedium& operator=(FakeWebRtcMedium&&) = delete;
  ~FakeWebRtcMedium() override;

  // WebRtcMedium:
  bool IsValid() const override { return is_valid_; }
  std::unique_ptr<WebRtcSignalingMessenger> GetSignalingMessenger(
      absl::string_view self_id,
      const location::nearby::connections::LocationHint& location_hint)
      override;

  void TriggerCancellationDuringGetSignalingMessenger() {
    cancel_during_get_signaling_messenger_ = true;
  }
  void SetIsValid(bool is_valid) { is_valid_ = is_valid; }
  FakeWebRtcSignalingMessenger* GetMostRecentSignalingMessenger() {
    return most_recent_signaling_messenger_;
  }

 private:
  CancellationFlag* flag_ = nullptr;
  bool is_valid_ = true;
  bool cancel_during_get_signaling_messenger_ = false;
  FakeWebRtcSignalingMessenger* most_recent_signaling_messenger_ = nullptr;
};
}  // namespace nearby

#endif  // THIRD_PARTY_NEARBY_INTERNAL_TEST_FAKE_WEBRTC_H_
