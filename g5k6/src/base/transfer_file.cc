#include "base/transfer_file.h"

TransferFile::TransferFile(SocketServer* server)
    : server_(server) {
}

TransferFile::~TransferFile() {
}

int32_t TransferFile::Send(uint8_t* data, int32_t length) {
    return -1;
}
