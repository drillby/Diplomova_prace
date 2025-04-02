import csv  # Add import for CSV
import sys
import time
from collections import deque

import pyqtgraph as pg
import serial
from pyqtgraph.Qt import QtCore, QtWidgets

# Parametry sériové komunikace
PORT = "/dev/ttyACM0"
BAUDRATE = 9600

# Inicializace sériového portu
ser = serial.Serial(PORT, BAUDRATE)
time.sleep(2)

# Parametry grafu
WINDOW_WIDTH_SEC = 5  # okno v čase
SAMPLING_INTERVAL = 0.01  # očekávaný interval vzorkování (100 Hz)
MAX_SAMPLES = int(WINDOW_WIDTH_SEC / SAMPLING_INTERVAL)

# Inicializace dat
voltages = deque([0] * MAX_SAMPLES, maxlen=MAX_SAMPLES)
timestamps = [i * SAMPLING_INTERVAL for i in range(MAX_SAMPLES)]  # fixní osa X

# Aplikace
app = QtWidgets.QApplication(sys.argv)
win = pg.GraphicsLayoutWidget(title="EMG signál v reálném čase")
win.show()
win.setWindowTitle("EMG Realtime Plot")

plot = win.addPlot(title="Napětí vs čas")
plot.setLabel("left", "Napětí", units="ADC")
plot.setLabel("bottom", "Čas", units="s")
plot.setYRange(250, 400)
plot.setXRange(WINDOW_WIDTH_SEC, 0)
plot.showGrid(x=True, y=True)

curve = plot.plot(timestamps, list(voltages))


def update():
    while ser.in_waiting:
        try:
            line = ser.readline().decode("utf-8").strip()
            voltage = float(line)
            voltages.append(voltage)
        except:
            pass

    curve.setData(timestamps, list(voltages))


def save_to_csv():
    """Save the last 2 seconds of data to a CSV file."""
    with open(f"last_{WINDOW_WIDTH_SEC}_seconds.csv", "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["Timestamp (s)", "Voltage (ADC)"])
        for t, v in zip(timestamps, voltages):
            writer.writerow([t, v])


# Timer pro pravidelnou aktualizaci
timer = QtCore.QTimer()
timer.timeout.connect(update)
timer.start(int(SAMPLING_INTERVAL * 1000))  # v ms

# Spuštění aplikace
if __name__ == "__main__":
    app.aboutToQuit.connect(save_to_csv)  # Connect save_to_csv to app close event
    QtWidgets.QApplication.instance().exec_()
