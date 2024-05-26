import subprocess
import re


class ApiClient:
    def __init__(self, executable_path='./code', command_counter=1):
        self.executable_path = executable_path
        self.process = subprocess.Popen(
            [self.executable_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        self.command_counter = command_counter

    def __del__(self):
        if self.process and self.process.poll() is None:
            self.exit()

    def send_command(self, command):
        command = f"[{self.command_counter}] {command}"
        print("Sending command:", command)
        self.process.stdin.write(command + '\n')
        self.process.stdin.flush()

        output_lines = []
        while True:
            output = self.process.stdout.readline().strip()
            if not output:  # An empty line indicates the end of the output
                break
            output_lines.append(output)

        # Combine all lines into a single string, joining with newlines
        output = "\n".join(output_lines)
        expected_prefix = f"[{self.command_counter}]"
        if not output.startswith(expected_prefix):
            raise ValueError(f"Expected output to start with '{expected_prefix}', but got '{output}'")

        self.command_counter += 1
        return output[len(expected_prefix) + 1:]  # Remove the prefix and the following space

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
        if not re.match(r'^[\u4e00-\u9fa5]{2,5}$', name):
            raise ValueError("Name must be 2 to 5 Chinese characters")
        return name

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

    def exit(self):
        command = f"exit"
        output = self.send_command(command)
        self.process.stdin.close()
        self.process.stdout.close()
        self.process.stderr.close()
        self.process.terminate()
        return output


if __name__ == "__main__":
    # Instantiate the class with the path to the executable
    tts = ApiClient('./code')

    # Use the methods to interact with the system
    print(tts.clean())
    print(tts.add_user('', 'admin', 'admin123', '管理员', 'admin@example.com', 10))
    print(tts.login('admin', 'admin123'))
    print(tts.add_user('admin', 'user1', 'user123', '用户一', 'user1@example.com', 1))
    print(tts.query_profile('admin', 'user1'))
    print(tts.logout('admin'))
    print(tts.exit())
