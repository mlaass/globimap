from http.server import HTTPServer, BaseHTTPRequestHandler
import ssl


httpd = HTTPServer(('0.0.0.0', 443), BaseHTTPRequestHandler)

httpd.socket = ssl.wrap_socket (httpd.socket, 
        keyfile="key.pem", 
        certfile='cert.pem', server_side=True, ssl_version=ssl.PROTOCOL_TLS)

httpd.serve_forever()
