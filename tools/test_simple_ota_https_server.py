import contextlib
import http.client
import io
import os
from pathlib import Path
import shutil
import ssl
import subprocess
import tempfile
import threading
import unittest

import simple_ota_https_server as ota


class CommandLineTests(unittest.TestCase):
    def test_existing_positionals_and_safe_defaults(self):
        _, args = ota.parse_arguments(['build', '8443', '../certificates'])
        self.assertEqual(args.image_dir, Path('build'))
        self.assertEqual(args.cert_dir, Path('../certificates'))
        self.assertEqual(args.port, 8443)
        self.assertEqual(args.bind, '127.0.0.1')
        self.assertEqual(args.file, 'diagral-uart-esp32.bin')

    def test_invalid_port(self):
        with contextlib.redirect_stderr(io.StringIO()):
            for port in ('0', '65536'):
                with self.subTest(port=port), self.assertRaises(SystemExit):
                    ota.parse_arguments(['build', port])


class HttpsServerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        openssl = shutil.which('openssl')
        if openssl is None:
            candidate = Path(os.environ.get('ProgramFiles', '')) / 'Git/usr/bin/openssl.exe'
            openssl = str(candidate) if candidate.is_file() else None
        if openssl is None:
            raise unittest.SkipTest('OpenSSL is required to generate an isolated test certificate')
        cls.temporary = tempfile.TemporaryDirectory(prefix='diagral-ota-tests-')
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.root = Path(cls.temporary.name)
        cls.certificates = cls.root / 'certificates'
        cls.certificates.mkdir()
        cls.certificate = cls.certificates / 'ca_cert.pem'
        cls.private_key = cls.certificates / 'ca_key.pem'
        subprocess.run([
            openssl, 'req', '-x509', '-newkey', 'rsa:2048', '-sha256', '-nodes',
            '-days', '1', '-keyout', str(cls.private_key), '-out', str(cls.certificate),
            '-subj', '/CN=127.0.0.1', '-addext', 'subjectAltName=IP:127.0.0.1',
        ], check=True, capture_output=True)
        cls.context = ssl.create_default_context(cafile=str(cls.certificate))

    def setUp(self):
        self.image_directory = Path(tempfile.mkdtemp(dir=self.root, prefix='images '))
        self.firmware = self.image_directory / 'diagral-uart-esp32.bin'
        self.firmware.write_bytes(b'test application firmware')

    def create_server(self, **options):
        return ota.create_https_server(
            str(self.image_directory), '127.0.0.1', 0,
            str(self.certificate), str(self.private_key), **options,
        )

    def running_server(self, **options):
        server = self.create_server(**options)
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()

        def cleanup():
            server.shutdown()
            server.server_close()
            worker.join(timeout=5)

        self.addCleanup(cleanup)
        return server

    def request(self, server, method, path, context=None):
        connection = http.client.HTTPSConnection(
            '127.0.0.1', server.server_port, context=context or self.context, timeout=5,
        )
        try:
            connection.request(method, path)
            response = connection.getresponse()
            return response.status, dict(response.getheaders()), response.read()
        finally:
            connection.close()

    def test_https_get_and_head(self):
        server = self.running_server()
        status, headers, payload = self.request(server, 'GET', '/diagral-uart-esp32.bin')
        self.assertEqual(status, 200)
        self.assertEqual(payload, self.firmware.read_bytes())
        self.assertEqual(int(headers['Content-Length']), len(payload))
        status, headers, payload = self.request(server, 'HEAD', '/diagral-uart-esp32.bin')
        self.assertEqual(status, 200)
        self.assertEqual(payload, b'')
        self.assertEqual(int(headers['Content-Length']), self.firmware.stat().st_size)

    def test_only_selected_binary_is_served(self):
        shutil.copyfile(self.certificate, self.image_directory / 'ca_cert.pem')
        shutil.copyfile(self.private_key, self.image_directory / 'ca_key.pem')
        (self.image_directory / 'bootloader.bin').write_bytes(b'not the application')
        server = self.running_server()
        for path in ('/', '/ca_cert.pem', '/ca_key.pem', '/../certificates/ca_key.pem',
                     '/%2e%2e/certificates/ca_key.pem', '/bootloader.bin'):
            for method in ('GET', 'HEAD'):
                with self.subTest(path=path, method=method):
                    self.assertEqual(self.request(server, method, path)[0], 404)

    def test_custom_binary_filename(self):
        custom = self.image_directory / 'custom.bin'
        custom.write_bytes(b'custom firmware')
        server = self.running_server(firmware_file=custom.name)
        self.assertEqual(self.request(server, 'GET', '/custom.bin')[2], b'custom firmware')
        self.assertEqual(self.request(server, 'GET', '/diagral-uart-esp32.bin')[0], 404)

    def test_subnet_filter(self):
        allowed = self.running_server(allowed_subnet='127.0.0.0/8')
        self.assertEqual(self.request(allowed, 'GET', '/diagral-uart-esp32.bin')[0], 200)
        denied = self.running_server(allowed_subnet='192.0.2.0/24')
        self.assertEqual(self.request(denied, 'GET', '/diagral-uart-esp32.bin')[0], 403)

    def test_untrusted_certificate_is_rejected(self):
        server = self.running_server()
        with self.assertRaises(ssl.SSLCertVerificationError):
            self.request(server, 'GET', '/diagral-uart-esp32.bin', ssl.create_default_context())

    def test_relative_paths_do_not_change_working_directory(self):
        previous = Path.cwd()
        os.chdir(self.root)
        try:
            server = ota.create_https_server(
                self.image_directory.name, '127.0.0.1', 0,
                'certificates/ca_cert.pem', 'certificates/ca_key.pem',
            )
            try:
                self.assertEqual(Path.cwd(), self.root)
            finally:
                server.server_close()
        finally:
            os.chdir(previous)

    def test_missing_certificate_creates_or_overwrites_nothing(self):
        original_key = self.private_key.read_bytes()
        original_entries = set(self.image_directory.iterdir())
        with self.assertRaises(FileNotFoundError):
            ota.create_https_server(str(self.image_directory), '127.0.0.1', 0)
        self.assertEqual(set(self.image_directory.iterdir()), original_entries)
        self.assertEqual(self.private_key.read_bytes(), original_key)

    def test_invalid_firmware_filename(self):
        for filename in ('../firmware.bin', '..\\firmware.bin', 'ca_key.pem'):
            with self.subTest(filename=filename), self.assertRaises(ValueError):
                self.create_server(firmware_file=filename)

    def test_symlink_is_not_served(self):
        external = self.certificates / 'external.bin'
        external.write_bytes(b'external file')
        link = self.image_directory / 'linked.bin'
        try:
            link.symlink_to(external)
        except OSError:
            self.skipTest('This environment does not allow creating symlinks')
        with self.assertRaises(ValueError):
            self.create_server(firmware_file=link.name)


if __name__ == '__main__':
    unittest.main()