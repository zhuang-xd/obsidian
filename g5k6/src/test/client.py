import os
import sys
import socket
import struct
import threading, time
from ctypes import Structure, c_uint32, c_uint8, c_uint16, c_int32, sizeof

BUFFER_SIZE = 10240
SERVER_PORT = 8556
APP_MAGIC_NUMBER = 0xAF8C
APP_COMMAND_SEND_FILE = 1
APP_COMMAND_FILE_LIST = 3
APP_COMMAND_ARCH_UPGRADE = 4
APP_COMMAND_LINUX_UPGRADE = 5
APP_DEFAULT_CRC = 0xAF8C
SERVER_IP = "192.168.3.251"

SEND_PATH = "/home/cle001/code/releases/bin"

SAVE_PATH = "/home/cle001/code/releases/bin"

# 定义与C端对应的结构体
class FileStreamHeader(Structure):
    _pack_ = 1  #
    _fields_ = [
    ("magic", c_uint16),
    ("version", c_uint16),
    # command type
    ("command", c_uint16),

    ("source_id", c_uint16),

    ("target_id", c_uint16),

    ("reserved", c_uint16),

    # external data length in front of data
    ("external_length", c_uint16),

    # header+paylaod+crc
    ("total_length", c_uint32),
    ]

FRAME_HEADER_SIZE = sizeof(FileStreamHeader)
FRAME_CRC_SIZE = sizeof(c_uint16)

class FileClient:
    def __init__(self, host='127.0.0.1', port=SERVER_PORT):
        self.host = host
        self.port = port
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def start(self):
        print(f"Server connecting on {self.host}:{self.port}")
        self.client_socket.connect((self.host, self.port))
        print(f"已连接到服务端 {self.host}:{self.port}")
        # self.request_file_list(self.client_socket)
        self.request_linux_upgrade(self.client_socket)
        self.client_socket.close()

    def request_file_list(self, client_sock):
        header = FileStreamHeader()
        header.magic = socket.htons(APP_MAGIC_NUMBER)
        header.command = socket.htons(APP_COMMAND_FILE_LIST)
        header.total_length = FRAME_HEADER_SIZE + FRAME_CRC_SIZE
        crc_value = APP_DEFAULT_CRC
        crc_bytes = crc_value.to_bytes(2, 'little')  # CRC校验值（4字节）
        sending_bytes = client_sock.send(bytes(header))
        print(f"client sending header server{sending_bytes}")
        sending_bytes = client_sock.send(crc_bytes)
        print(f"client sending crc server{sending_bytes}")
        self.handle_client(client_sock)

    def receive_stream_header(self, client_sock):
        # 接收头部数据
        # print(f"client receive_stream_header {client_sock}, header_size:{header_size}")
        header_data = client_sock.recv(FRAME_HEADER_SIZE)
        print(f"client receive_stream_header header_data:{len(header_data)}")
        if len(header_data) != FRAME_HEADER_SIZE:
            raise Exception("Invalid header size")
        header = FileStreamHeader.from_buffer_copy(header_data)
        header.command = socket.ntohs(header.command)
        header.external_length = socket.ntohs(header.external_length)
        header.total_length = socket.ntohl(header.total_length)
        return header

    def handle_client(self, client_sock):
        try:
            # 接收并解析头部
            header = self.receive_stream_header(client_sock)
            print(f"handling command: {header.command}")
            if header.command == APP_COMMAND_FILE_LIST:  # 发送文件
                # 接收文件名
                payload_size = header.total_length - FRAME_CRC_SIZE - FRAME_HEADER_SIZE
                filename_data = []
                filename = ""
                if payload_size > 0:
                    filename_data = client_sock.recv(payload_size)
                    filename = filename_data.decode('utf-8')
                crc_data = client_sock.recv(FRAME_CRC_SIZE)
                crc_value = int.from_bytes(crc_data, byteorder='little')
                print(f"handling server: {filename}, crc_value:{crc_value}, APP_DEFAULT_CRC:{APP_DEFAULT_CRC}")
                if crc_value == APP_DEFAULT_CRC:
                    print(f"crc value: {crc_value.to_bytes(2, 'big').hex()}")
                # filename = "a.h264"
                if filename:
                    self.request_file(client_sock, filename)
                else:
                    print("file path empty")
        except Exception as e:
            print(f"Error handling server: {e}")
        finally:
            client_sock.close()

    def request_file(self, client_sock, file_name):
        print(f"request_file {file_name}")
        header = FileStreamHeader()
        header.magic = socket.htons(APP_MAGIC_NUMBER)
        header.command = socket.htons(APP_COMMAND_SEND_FILE)
        file_name_data = file_name.encode('utf-8')
        payload_length = len(file_name_data)
        header.total_length = socket.htonl(FRAME_HEADER_SIZE + FRAME_CRC_SIZE + payload_length)
        header.total_length = socket.htonl(FRAME_HEADER_SIZE + FRAME_CRC_SIZE + payload_length)
        crc_value = APP_DEFAULT_CRC
        print(f"request_file crc_value{crc_value}")
        crc_bytes = crc_value.to_bytes(2, 'little')  # CRC校验值（4字节）
        send_data = bytes(header) + file_name_data + crc_bytes
        sending_bytes = client_sock.send(send_data)
        print(f"request_file sending len:{len(send_data)} server{sending_bytes}")
        self.read_file_data(client_sock)

    def read_file_data(self, client_sock):
        try:
            # 接收并解析头部
            header = self.receive_stream_header(client_sock)
            if header.command == APP_COMMAND_SEND_FILE:  # 发送文件
                # 接收文件名
                file_name_length = header.external_length
                filename_data = client_sock.recv(file_name_length)
                print(f"handling server file_name_length: {len(filename_data)}")

                file_name = filename_data.decode('utf-8')
                file_size = header.total_length - FRAME_HEADER_SIZE - file_name_length - FRAME_CRC_SIZE
                print(f"handling server filename: {file_name}, filesize: {file_size}")
                get_size = 0
                sent_size = 0
                file_path = "/home/cle001/code/releases/" + file_name
                with open(file_path, 'wb') as f:
                    while get_size < file_size:
                        if  (file_size - get_size) > BUFFER_SIZE:
                            buffer_size = BUFFER_SIZE
                        else:
                            buffer_size = (file_size - get_size)

                        onece_data = client_sock.recv(buffer_size)
                        if len(onece_data) == 0:
                            os.sleep(0.001)
                            continue
                        data = f.write(onece_data)
                        get_size += len(onece_data)

                crc_data = client_sock.recv(FRAME_CRC_SIZE)
                filename = filename_data.decode('utf-8')
                print(f"handling server: {filename}, file_data:{get_size}")
                crc_value = int.from_bytes(crc_data, byteorder='little')
                print(f"handling server: {filename}, crc_value:{crc_value}, APP_DEFAULT_CRC:{APP_DEFAULT_CRC}")
                if crc_value == APP_DEFAULT_CRC:
                    print(f"crc value: {crc_value.to_bytes(2, 'big').hex()}")
            else:  # 获取文件
                # 接收文件名
                filename_data = client_sock.recv(header.total_length - FRAME_CRC_SIZE - FRAME_HEADER_SIZE)
                filename = filename_data.decode('utf-8')
                print(f"handling server: {filename}")
                # self.receive_file(client_sock, filename, header.fileSize)
        except Exception as e:
            print(f"Error handling server: {e}")
        finally:
            client_sock.close()

    def request_linux_upgrade(self, client_sock):
        file_name = "http://192.168.3.244:5500/uImage.img"
        print(f"request_file {file_name}")
        header = FileStreamHeader()
        header.magic = socket.htons(APP_MAGIC_NUMBER)
        header.command = socket.htons(APP_COMMAND_LINUX_UPGRADE)
        file_name_data = file_name.encode('utf-8')
        payload_length = len(file_name_data)
        header.total_length = socket.htonl(FRAME_HEADER_SIZE + FRAME_CRC_SIZE + payload_length)
        header.total_length = socket.htonl(FRAME_HEADER_SIZE + FRAME_CRC_SIZE + payload_length)
        crc_value = APP_DEFAULT_CRC
        print(f"request_file crc_value{crc_value}")
        crc_bytes = crc_value.to_bytes(2, 'little')  # CRC校验值（4字节）
        send_data = bytes(header) + file_name_data + crc_bytes
        sending_bytes = client_sock.send(send_data)
        print(f"request_file sending len:{len(send_data)} server{sending_bytes}")
        self.read_file_data(client_sock)

    def read_upgrade_data(self, client_sock):
        try:
            # 接收并解析头部
            header = self.receive_stream_header(client_sock)
            if header.command == APP_COMMAND_LINUX_UPGRADE:
                file_name_length = header.external_length
                filename_data = client_sock.recv(file_name_length)
                file_name = filename_data.decode('utf-8')
                print(f"upgrade_upgrade result: {file_name}")
        except Exception as e:
            print(f"Error handling server: {e}")
        finally:
            client_sock.close()

if __name__ == "__main__":
    server_ip = SERVER_IP
    if len(sys.argv) > 1:
        server_ip = sys.argv[1]
    client = FileClient(host=server_ip)
    client.start()
