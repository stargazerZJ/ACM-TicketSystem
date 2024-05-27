import socket


class ApiClient:
    def __init__(self, socket_path='/tmp/api_socket'):
        self.socket_path = socket_path

    def send_command(self, command):
        print("Sending command:", command)

        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client_socket:
            client_socket.connect(self.socket_path)
            client_socket.sendall(command.encode() + b'\n')

            output = client_socket.recv(1024).decode().strip()
        return output

    def exit(self):
        print("Sending exit command")
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client_socket:
            client_socket.connect(self.socket_path)
            client_socket.sendall(b"exit\n")
            response = client_socket.recv(1024).decode().strip()
        return response

    def validate_string(self, value, max_length, allow_spaces=False):
        if not value:
            raise ValueError("String cannot be empty")
        if len(value) > max_length:
            raise ValueError(f"String exceeds maximum length of {max_length}")
        if not allow_spaces and ' ' in value:
            raise ValueError("String contains spaces, which are not allowed")
        return value

    def validate_username(self, username):
        return self.validate_string(username, 20)

    def validate_password(self, password):
        return self.validate_string(password, 30, allow_spaces=True)

    def validate_name(self, name):
        self.validate_string(name, 15)

    def validate_mail(self, mail):
        return self.validate_string(mail, 30)

    def validate_trainID(self, trainID):
        return self.validate_string(trainID, 20)

    def validate_station(self, station):
        return self.validate_string(station, 10)

    def add_user(self, cur_username, username, password, name, mail_addr, privilege):
        username = self.validate_username(username)
        password = self.validate_password(password)
        name = self.validate_name(name)
        mail_addr = self.validate_mail(mail_addr)

        if not cur_username:
            # If cur_username is empty, it means we are adding the first user, so we use the default command
            command = f"add_user -c default -u {username} -p {password} -n {name} -m {mail_addr} -g 10"
        else:
            cur_username = self.validate_username(cur_username)
            command = f"add_user -c {cur_username} -u {username} -p {password} -n {name} -m {mail_addr} -g {privilege}"

        return self.send_command(command)

    def login(self, username, password):
        username = self.validate_username(username)
        password = self.validate_password(password)

        command = f"login -u {username} -p {password}"
        return self.send_command(command)

    def logout(self, username):
        username = self.validate_username(username)

        command = f"logout -u {username}"
        return self.send_command(command)

    def query_profile(self, cur_username, username):
        cur_username = self.validate_username(cur_username)
        username = self.validate_username(username)

        command = f"query_profile -c {cur_username} -u {username}"
        return self.send_command(command)

    def modify_profile(self, cur_username, username, password=None, name=None, mail_addr=None, privilege=None):
        cur_username = self.validate_username(cur_username)
        username = self.validate_username(username)
        if password:
            password = self.validate_password(password)
        if name:
            name = self.validate_name(name)
        if mail_addr:
            mail_addr = self.validate_mail(mail_addr)

        command = f"modify_profile -c {cur_username} -u {username}"
        if password:
            command += f" -p {password}"
        if name:
            command += f" -n {name}"
        if mail_addr:
            command += f" -m {mail_addr}"
        if privilege is not None:
            command += f" -g {privilege}"
        return self.send_command(command)

    def add_train(self, trainID, stationNum, seatNum, stations, prices, startTime, travelTimes, stopoverTimes, saleDate,
                  trainType):
        trainID = self.validate_trainID(trainID)
        stations = '|'.join([self.validate_station(s) for s in stations.split('|')])

        command = f"add_train -i {trainID} -n {stationNum} -m {seatNum} -s {stations} -p {prices} -x {startTime} -t {travelTimes} -o {stopoverTimes} -d {saleDate} -y {trainType}"
        return self.send_command(command)

    def delete_train(self, trainID):
        trainID = self.validate_trainID(trainID)

        command = f"delete_train -i {trainID}"
        return self.send_command(command)

    def release_train(self, trainID):
        trainID = self.validate_trainID(trainID)

        command = f"release_train -i {trainID}"
        return self.send_command(command)

    def query_train(self, trainID, date):
        trainID = self.validate_trainID(trainID)

        command = f"query_train -i {trainID} -d {date}"
        return self.send_command(command)

    def query_ticket(self, start_station, end_station, date, priority='time'):
        start_station = self.validate_station(start_station)
        end_station = self.validate_station(end_station)

        command = f"query_ticket -s {start_station} -t {end_station} -d {date} -p {priority}"
        return self.send_command(command)

    def query_transfer(self, start_station, end_station, date, priority='time'):
        start_station = self.validate_station(start_station)
        end_station = self.validate_station(end_station)

        command = f"query_transfer -s {start_station} -t {end_station} -d {date} -p {priority}"
        return self.send_command(command)

    def buy_ticket(self, username, trainID, date, num_tickets, from_station, to_station, queue=False):
        username = self.validate_username(username)
        trainID = self.validate_trainID(trainID)
        from_station = self.validate_station(from_station)
        to_station = self.validate_station(to_station)

        command = f"buy_ticket -u {username} -i {trainID} -d {date} -n {num_tickets} -f {from_station} -t {to_station}"
        if queue:
            command += " -q true"
        return self.send_command(command)

    def query_order(self, username):
        username = self.validate_username(username)

        command = f"query_order -u {username}"
        return self.send_command(command)

    def refund_ticket(self, username, order_num=1):
        username = self.validate_username(username)

        command = f"refund_ticket -u {username} -n {order_num}"
        return self.send_command(command)

    def clean(self):
        command = f"clean"
        return self.send_command(command)

if __name__ == "__main__":
    # Instantiate the class
    tts = ApiClient('/tmp/api_socket')

    # Use the methods to interact with the system
    print(tts.clean())
    print(tts.add_user('', 'admin', 'admin123', '管理员', 'admin@example.com', 10))
    print(tts.login('admin', 'admin123'))
    print(tts.add_user('admin', 'user1', 'user123', '用户一', 'user1@example.com', 1))
    print(tts.query_profile('admin', 'user1'))
    print(tts.logout('admin'))
    print(tts.exit())
