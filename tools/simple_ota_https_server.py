# SPDX-FileCopyrightText: 2022-2025 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Unlicense OR CC0-1.0
import http.server
import os
import ssl
import sys

def start_https_server(
    ota_image_dir: str,
    server_ip: str,
    server_port: int,
    server_file: str | None = None,
    key_file: str | None = None,
) -> None:
    os.chdir(ota_image_dir)

    if server_file is None:
        server_file = os.path.join(ota_image_dir, 'server_cert.pem')
        cert_file_handle = open(server_file, 'w+', encoding='utf-8')
        cert_file_handle.write(server_cert)
        cert_file_handle.close()

    if key_file is None:
        key_file = os.path.join(ota_image_dir, 'server_key.pem')
        key_file_handle = open('server_key.pem', 'w+', encoding='utf-8')
        key_file_handle.write(server_key)
        key_file_handle.close()

    httpd = http.server.HTTPServer((server_ip, server_port), http.server.SimpleHTTPRequestHandler)

    ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ssl_context.load_cert_chain(certfile=server_file, keyfile=key_file)

    httpd.socket = ssl_context.wrap_socket(httpd.socket, server_side=True)
    httpd.serve_forever()

if __name__ == '__main__':
    if sys.argv[2:]:  # if two or more arguments provided:
        # Usage: pytest_simple_ota.py <image_dir> <server_port> [cert_dir]
        this_dir = os.path.dirname(os.path.realpath(__file__))
        bin_dir = os.path.join(this_dir, sys.argv[1])
        port = int(sys.argv[2])
        cert_dir = bin_dir if not sys.argv[3:] else os.path.join(this_dir, sys.argv[3])  # optional argument
        print(f'Starting HTTPS server at "https://0.0.0.0:{port}"')
        start_https_server(
            bin_dir,
            '',
            port,
            server_file=os.path.join(cert_dir, 'ca_cert.pem'),
            key_file=os.path.join(cert_dir, 'ca_key.pem'),
        )
