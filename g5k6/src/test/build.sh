#!/bin/bash -e

g++ ../base/socket_server.cc ../base/transfer_handler.cc ../base/file_path.cc ../base/file_enumerator.cc ../test/test.cc  -ggdb3 -I../