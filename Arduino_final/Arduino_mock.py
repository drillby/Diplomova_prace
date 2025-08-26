import socket
import threading
import time

# --- Konstanty ---
TCP_PORT = 3000
REFRESH_RATE_HZ = 1
REFRESH_INTERVAL = 1.0 / REFRESH_RATE_HZ
HEARTBEAT_INTERVAL = 1.5


class EMGSystem:
    def __init__(self, port):
        self.port = port
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket = None
        self.client_address = None
        self.initialized = False
        self.heartbeat_running = False
        self.manual_value = None  # Hodnota zadaná uživatelem
        self.last_sent_value = -1

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
            # 1) Očekává se první zpráva: HANDSHAKE_REQUEST
            self.client_socket.settimeout(3.0)
            first_msg = self.client_socket.recv(64).decode().strip()
            print(f"Přijatý vstup: {first_msg}")

            if first_msg != "HANDSHAKE_REQUEST":
                print("Neplatná handshake zpráva. Očekávám HANDSHAKE_REQUEST.")
                self.client_socket.close()
                return

            # 2) Odpověď na handshake
            # self.client_socket.sendall(b"HANDSHAKE\n")
            # print("Odesláno: HANDSHAKE")

            # 3) Přechod do běžného režimu
            self.client_socket.settimeout(None)
            self.initialized = True
            self.heartbeat_running = True

            # Pokud chceš heartbeat, odkomentuj řádek níže
            # threading.Thread(target=self.send_heartbeat, daemon=True).start()

            # Zahájíme vstup od uživatele (pevný rozsah 0–8) a smyčku odesílání
            threading.Thread(target=self.read_input_loop, daemon=True).start()
            self.send_loop()

        except (ValueError, socket.timeout) as e:
            print("Chyba během čtení od klienta:", e)
            self.client_socket.close()

    def send_loop(self):
        try:
            while self.initialized and self.client_socket:
                # kontrola příkazu DISCONNECT od klienta (neblokující)
                self.client_socket.setblocking(False)
                try:
                    cmd = self.client_socket.recv(64).decode()
                    if "DISCONNECT" in cmd:
                        print("Klient odeslal DISCONNECT.")
                        break
                except BlockingIOError:
                    pass
                finally:
                    self.client_socket.setblocking(True)

                # Odesílání nové hodnoty, pokud se změnila
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
                    self.client_socket.sendall(b"ALIVE\n")
                    print("Odesláno: ALIVE")
            except Exception as e:
                print("Chyba při odesílání ALIVE:", e)
                break

    def read_input_loop(self):
        max_val = 8  # pevný rozsah 0–8
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
