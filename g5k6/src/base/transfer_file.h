#ifndef BASE_TRANSFER_FILE_H_
#define BASE_TRANSFER_FILE_H_

#include <cstdint>

class SocketServer;

class TransferFile {
public:
    TransferFile(SocketServer* server);
    ~TransferFile();

public:
    int32_t Send(uint8_t* data, int32_t length);

private:
    SocketServer* server_{nullptr};
};

#endif  // BASE_TRANSFER_FILE_H_
