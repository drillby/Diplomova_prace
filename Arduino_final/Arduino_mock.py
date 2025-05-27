import socket
import threading
import time

# --- Konstanty ---
TCP_PORT = 8888
REFRESH_RATE_HZ = 1
REFRESH_INTERVAL = 1.0 / REFRESH_RATE_HZ
HEARTBEAT_INTERVAL = 1.5
MAX_SENSORS = 4


class EMGSystem:
    def __init__(self, port):
        self.port = port
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket = None
        self.client_address = None
        self.num_sensors = 0
        self.last_sent_value = -1
        self.initialized = False
        self.heartbeat_running = False
        self.manual_value = None  # Hodnota zadaná uživatelem

    def start_server(self):
        self.server_socket.bind(("", self.port))
        self.server_socket.listen(1)
        print(f"Server spuštěn na portu {self.port}")
        threading.Thread(target=self.accept_clients, daemon=True).start()

    def accept_clients(self):
        while True:
            client_socket, addr = self.server_socket.accept()
            print(f"Klient připojen: {addr}")
            self.client_socket = client_socket
            self.client_address = addr
            self.handle_client()

    def handle_client(self):
        try:
            self.client_socket.settimeout(3.0)
            input_data = self.client_socket.recv(10).decode().strip()
            print(f"Přijatý vstup: {input_data}")
            count = int(input_data)

            if 1 <= count <= MAX_SENSORS:
                self.num_sensors = count
                self.initialized = True
                self.client_socket.settimeout(None)

                self.heartbeat_running = True
                threading.Thread(target=self.send_heartbeat, daemon=True).start()
                threading.Thread(target=self.read_input_loop, daemon=True).start()

                self.send_loop()
            else:
                print("Neplatný počet senzorů.")
                self.client_socket.close()

        except (ValueError, socket.timeout) as e:
            print("Chyba během čtení od klienta:", e)
            self.client_socket.close()

    def send_loop(self):
        try:
            while self.initialized and self.client_socket:
                # kontrola příkazu DISCONNECT
                self.client_socket.setblocking(False)
                try:
                    cmd = self.client_socket.recv(64).decode()
                    if "DISCONNECT" in cmd:
                        print("Klient odeslal DISCONNECT.")
                        break
                except BlockingIOError:
                    pass
                self.client_socket.setblocking(True)

                if (
                    self.manual_value is not None
                    and self.manual_value != self.last_sent_value
                ):
                    self.last_sent_value = self.manual_value
                    msg = f"{self.manual_value}\n"
                    self.client_socket.sendall(msg.encode())
                    print(f"Odesláno: {msg.strip()}")

                time.sleep(REFRESH_INTERVAL)

        except Exception as e:
            print("Chyba během komunikace:", e)
        finally:
            self.cleanup_client()

    def send_heartbeat(self):
        while self.heartbeat_running and self.client_socket:
            try:
                time.sleep(HEARTBEAT_INTERVAL)
                if self.client_socket:
                    self.client_socket.sendall(b"HEART BEAT\n")
                    print("Odesláno: HEART BEAT")
            except Exception as e:
                print("Chyba při odesílání HEART BEAT:", e)
                break

    def read_input_loop(self):
        max_val = 2**self.num_sensors - 1
        print(f"Zadej číslo v rozsahu 0 až {max_val} pro odeslání:")
        while self.initialized:
            try:
                user_input = input("> ").strip()
                if not user_input:
                    continue
                value = int(user_input)
                if 0 <= value <= max_val:
                    self.manual_value = value
                else:
                    print(f"Neplatná hodnota, zadej číslo mezi 0 a {max_val}")
            except ValueError:
                print("Zadej platné celé číslo.")

    def cleanup_client(self):
        self.heartbeat_running = False
        if self.client_socket:
            self.client_socket.close()
        self.client_socket = None
        self.initialized = False
        self.last_sent_value = -1
        self.manual_value = None
        print("Klient odpojen a systém resetován.")


if __name__ == "__main__":
    emg = EMGSystem(TCP_PORT)
    emg.start_server()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Server ukončen.")
