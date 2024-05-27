import subprocess
import socket
import os
import time


class ApiServer:
    def __init__(self, executable_path='./code', socket_path='/tmp/api_socket'):
        self.executable_path = executable_path
        self.socket_path = socket_path
        self.command_counter = int(time.time() - time.mktime(time.strptime(time.strftime("%Y-%m-%d"), "%Y-%m-%d")))
        self.process = subprocess.Popen(
            [self.executable_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        self.server_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.server_socket.bind(self.socket_path)
        self.server_socket.listen(1)
        self.is_running = True

    def __del__(self):
        self.exit()
        self.cleanup_socket()

    def start(self):
        try:
            while self.is_running:
                client_socket, _ = self.server_socket.accept()
                try:
                    while True:
                        command = client_socket.recv(1024).decode()
                        if not command:
                            break
                        print("Receive command:", command)
                        if command.strip() == "exit":
                            response = self.exit()
                            client_socket.sendall(response.encode() + b"\n")
                            self.is_running = False
                            break
                        response = self.handle_command(command.strip())
                        print("Got response:", response)
                        client_socket.sendall(response.encode() + b"\n")
                finally:
                    client_socket.close()
        except Exception as e:
            print(f"Server error: {e}")
        finally:
            self.cleanup_socket()

    def cleanup_socket(self):
        if os.path.exists(self.socket_path):
            os.remove(self.socket_path)

    def handle_command(self, command):
        prefixed_command = f"[{self.command_counter}] {command}"
        return self.send_command(prefixed_command)

    def send_command(self, command):
        self.process.stdin.write(command + '\n')
        self.process.stdin.flush()

        output_lines = []
        while True:
            output = self.process.stdout.readline().strip()
            if not output:
                break
            output_lines.append(output)

        output = "\n".join(output_lines)
        expected_prefix = f"[{self.command_counter}]"
        if not output.startswith(expected_prefix):
            raise ValueError(f"Expected output to start with '{expected_prefix}', but got '{output}'")

        self.command_counter += 1
        return output[len(expected_prefix) + 1:]  # Remove the prefix and the following space

    def exit(self):
        if not self.is_running:
            return "Server is already stopped"
        self.process.stdin.write('[1] exit\n')
        self.process.stdin.flush()

        # Allow the executable to terminate itself
        output_lines = []
        while True:
            output = self.process.stdout.readline().strip()
            print("Exit output:", output)
            if not output:
                break
            output_lines.append(output)

        # Close the subprocess streams
        self.process.stdin.close()
        self.process.stdout.close()
        self.process.stderr.close()

        # Wait for the process to terminate gracefully
        self.process.wait()

        # Ensure the server stops accepting new connections
        self.is_running = False

        output = "\n".join(output_lines)
        expected_prefix = "[1]"
        if not output.startswith(expected_prefix):
            raise ValueError(f"Expected output to start with '{expected_prefix}', but got '{output}'")
        return output[len(expected_prefix) + 1:]


if __name__ == "__main__":
    server = ApiServer(executable_path='./code', socket_path='/tmp/api_socket')
    try:
        print("Server starting.")
        server.start()
    except KeyboardInterrupt:
        print("Server shutting down.")
    finally:
        del server
