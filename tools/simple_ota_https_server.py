# SPDX-FileCopyrightText: 2022-2025 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Unlicense OR CC0-1.0
import argparse
import http.server
import ipaddress
from functools import partial
from pathlib import Path
import ssl
from urllib.parse import quote, urlsplit


class FirmwareRequestHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, directory, firmware_file, allowed_subnet=None, **kwargs):
        self.firmware_file = firmware_file
        self.allowed_subnet = allowed_subnet
        super().__init__(*args, directory=directory, **kwargs)

    def permitted(self):
        if self.allowed_subnet is not None:
            if ipaddress.ip_address(self.client_address[0]) not in self.allowed_subnet:
                self.send_error(403)
                return False
        request = urlsplit(self.path)
        firmware_path = Path(self.directory) / self.firmware_file
        if (request.netloc or request.path != '/' + quote(self.firmware_file, safe='')
                or firmware_path.is_symlink()):
            self.send_error(404)
            return False
        return True

    def do_GET(self):
        if self.permitted():
            super().do_GET()

    def do_HEAD(self):
        if self.permitted():
            super().do_HEAD()


def create_https_server(
    ota_image_dir: str,
    server_ip: str,
    server_port: int,
    server_file: str | None = None,
    key_file: str | None = None,
    *,
    firmware_file: str = 'diagral-uart-esp32.bin',
    allowed_subnet: str | None = None,
) -> http.server.HTTPServer:
    image_dir = Path(ota_image_dir).resolve(strict=True)
    if not image_dir.is_dir():
        raise ValueError('The image directory must be a directory')
    if (not firmware_file.endswith('.bin') or '/' in firmware_file
            or '\\' in firmware_file):
        raise ValueError('The firmware file must be a .bin filename without a directory')
    firmware_path = image_dir / firmware_file
    if firmware_path.is_symlink() or not firmware_path.is_file():
        raise ValueError('The selected firmware must be an existing regular file, not a symlink')

    certificate = Path(server_file) if server_file is not None else image_dir / 'ca_cert.pem'
    private_key = Path(key_file) if key_file is not None else image_dir / 'ca_key.pem'
    certificate = certificate.resolve(strict=True)
    private_key = private_key.resolve(strict=True)
    network = ipaddress.ip_network(allowed_subnet, strict=False) if allowed_subnet else None

    ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ssl_context.minimum_version = ssl.TLSVersion.TLSv1_2
    ssl_context.load_cert_chain(certfile=certificate, keyfile=private_key)
    handler = partial(
        FirmwareRequestHandler,
        directory=str(image_dir),
        firmware_file=firmware_file,
        allowed_subnet=network,
    )
    httpd = http.server.HTTPServer((server_ip or '127.0.0.1', server_port), handler)
    try:
        httpd.socket = ssl_context.wrap_socket(httpd.socket, server_side=True)
    except Exception:
        httpd.server_close()
        raise
    return httpd

def start_https_server(
    ota_image_dir: str,
    server_ip: str,
    server_port: int,
    server_file: str | None = None,
    key_file: str | None = None,
    *,
    firmware_file: str = 'diagral-uart-esp32.bin',
    allowed_subnet: str | None = None,
) -> None:
    httpd = create_https_server(
        ota_image_dir, server_ip, server_port, server_file, key_file,
        firmware_file=firmware_file, allowed_subnet=allowed_subnet,
    )
    address, port = httpd.server_address
    print(f'Serving only {firmware_file} at https://{address}:{port}/{quote(firmware_file, safe="")}', flush=True)
    if address == '127.0.0.1':
        print('Loopback only; use --bind with the trusted LAN address for ESP32 access.', flush=True)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        httpd.server_close()


def parse_arguments(argv=None):
    parser = argparse.ArgumentParser(description='Serve one OTA binary over HTTPS; certificates are never served or generated.')
    parser.add_argument('image_dir', type=Path, help='Directory containing the firmware binary')
    parser.add_argument('port', type=int, help='HTTPS port (1-65535)')
    parser.add_argument('cert_dir', type=Path, nargs='?', help='Certificate directory; defaults to image_dir')
    parser.add_argument('--bind', default='127.0.0.1', help='Bind address; defaults to loopback. Select a trusted LAN IPv4 address for OTA.')
    parser.add_argument('--file', default='diagral-uart-esp32.bin', help='Firmware .bin filename to serve')
    parser.add_argument('--allow-subnet', help='Restrict HTTP requests to this client CIDR subnet, e.g. 192.168.1.0/24')
    args = parser.parse_args(argv)
    if not 1 <= args.port <= 65535:
        parser.error('port must be between 1 and 65535')
    return parser, args


def main():
    parser, args = parse_arguments()
    cert_dir = args.cert_dir if args.cert_dir is not None else args.image_dir
    try:
        start_https_server(
            str(args.image_dir), args.bind, args.port,
            server_file=str(cert_dir / 'ca_cert.pem'),
            key_file=str(cert_dir / 'ca_key.pem'),
            firmware_file=args.file,
            allowed_subnet=args.allow_subnet,
        )
    except (OSError, ValueError) as error:
        parser.error(str(error))


if __name__ == '__main__':
    main()
