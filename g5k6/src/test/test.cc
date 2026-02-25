#include "test.h"

#include "base/app_common_defs.h"
#include "base/transfer_file.h"
#include "base/logging.h"
#include "base/socket_server.h"
#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include <cstdint>
#include <stdio.h>
#include <string>
#include <unistd.h>


namespace {
// static constexpr uint16_t frameMaigicNumber =  EQMagic;
    static SocketServer *g_socket_server = nullptr;
inline std::string::size_type FinalExtensionSeparatorPosition(const std::string& path) {
  // Special case "." and ".."
  if (path == "." ||
      path == "..") {
    return std::string::npos;
  }

  return path.rfind("/");
}
};  //

int32_t test_cpp(void)
{
    auto print= [] (uint16_t aa) {
        LOG_M_I("ss", "%d\n", aa);
    };
    // TransferFile transfer(nullptr);
    // print(frameMaigicNumber);
    return 0;
}

void test_handle_sig(int32_t signo)
{
    if (g_socket_server) {
        g_socket_server->Stop();
    }
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {

    signal(SIGINT, test_handle_sig);
    signal(SIGQUIT, test_handle_sig);
    signal(SIGKILL, test_handle_sig);
    signal(SIGTERM, test_handle_sig);

    g_socket_server->Start("/home/cle001/code");
    while (1) {
        usleep(1000 * 50);
    }
}