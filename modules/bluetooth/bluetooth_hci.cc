//
// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#define LOG_TAG "android.hardware.bluetooth@1.0.hikey"

#include "bluetooth_hci.h"

#include <android-base/logging.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace hikey {

using android::hardware::hidl_vec;

BluetoothHci::BluetoothHci()
    : deathRecipient(new BluetoothDeathRecipient(this)) {}

Return<void> BluetoothHci::initialize(
    const ::android::sp<IBluetoothHciCallbacks>& cb) {
  ALOGI("BluetoothHci::initialize()");

  hci_tty_fd_ = open("/dev/hci_tty", O_RDWR);
  if (hci_tty_fd_ < 0) {
    ALOGE("%s: Can't open hci_tty (%s)", __func__, strerror(errno));
    cb->initializationComplete(Status::INITIALIZATION_ERROR);
    return Void();
  }

  event_cb_ = cb;
  event_cb_->linkToDeath(deathRecipient, 0);

  hci_ = new hci::H4Protocol(
      hci_tty_fd_,
      [cb](const hidl_vec<uint8_t>& packet) { cb->hciEventReceived(packet); },
      [cb](const hidl_vec<uint8_t>& packet) { cb->aclDataReceived(packet); },
      [cb](const hidl_vec<uint8_t>& packet) { cb->scoDataReceived(packet); });

  // Use a socket pair to enforce the TI FIONREAD requirement.
  int sockfd[2];
  socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd);
  int shim_fd = sockfd[0];
  int for_hci = sockfd[1];

  fd_watcher_.WatchFdForNonBlockingReads(hci_tty_fd_, [this, shim_fd](int fd) {
    int tty_bytes = 0;
    if (TEMP_FAILURE_RETRY(ioctl(fd, FIONREAD, &tty_bytes)))
      ALOGE("%s:FIONREAD %s", __func__, strerror(errno));
    ALOGV("%s:tty_bytes = %d", __func__, tty_bytes);

    uint8_t* tmp_buffer = new uint8_t[tty_bytes];
    size_t bytes_read = TEMP_FAILURE_RETRY(read(fd, tmp_buffer, tty_bytes));
    CHECK(static_cast<int>(bytes_read) == tty_bytes);
    size_t bytes_written =
        TEMP_FAILURE_RETRY(write(shim_fd, tmp_buffer, tty_bytes));
    CHECK(static_cast<int>(bytes_written) == tty_bytes);
    delete[] tmp_buffer;
  });

  fd_watcher_.WatchFdForNonBlockingReads(
      for_hci, [this](int fd) { hci_->OnDataReady(fd); });

  cb->initializationComplete(Status::SUCCESS);
  return Void();
}

Return<void> BluetoothHci::close() {
  ALOGI("BluetoothHci::close()");

  if (hci_tty_fd_ >= 0) {
    fd_watcher_.StopWatchingFileDescriptors();
    ::close(hci_tty_fd_);
    hci_tty_fd_ = -1;
  }

  event_cb_->unlinkToDeath(deathRecipient);

  if (hci_ != nullptr) {
    delete hci_;
    hci_ = nullptr;
  }

  return Void();
}

Return<void> BluetoothHci::sendHciCommand(const hidl_vec<uint8_t>& packet) {
  hci_->Send(HCI_PACKET_TYPE_COMMAND, packet.data(), packet.size());
  return Void();
}

Return<void> BluetoothHci::sendAclData(const hidl_vec<uint8_t>& packet) {
  hci_->Send(HCI_PACKET_TYPE_ACL_DATA, packet.data(), packet.size());
  return Void();
}

Return<void> BluetoothHci::sendScoData(const hidl_vec<uint8_t>& packet) {
  hci_->Send(HCI_PACKET_TYPE_SCO_DATA, packet.data(), packet.size());
  return Void();
}

}  // namespace hikey
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
