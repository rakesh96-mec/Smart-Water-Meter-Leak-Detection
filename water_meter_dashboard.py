from flask import Flask, jsonify, render_template_string
import serial
import threading
from collections import deque
from datetime import datetime

app = Flask(__name__)

# =========================
# SERIAL SETTINGS
# =========================
PORT = "COM6"
BAUD = 115200
ser = None
# Latest values received from STM32
data = {
    "pulses": 0,
    "volume": 0,
    "status": "NORMAL"
}

# History buffer — keeps last 200 readings
MAX_HISTORY = 200
history = deque(maxlen=MAX_HISTORY)

# =========================
# SERIAL READER (original logic, untouched)
# =========================
def serial_reader():
    global ser

    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
        while True:
            line = ser.readline().decode().strip()
            if not line:
                continue
            try:
                pulse, volume, status = line.split(",")
                data["pulses"] = pulse
                data["volume"] = volume
                data["status"] = status

                # Append timestamped reading to history
                history.append({
                    "time":   datetime.now().strftime("%H:%M:%S"),
                    "pulses": pulse,
                    "volume": volume,
                    "status": status
                })
            except:
                pass
    except Exception as e:
        print("Serial Error:", e)

# =========================
# WEB PAGE
# =========================
HTML = """
<!DOCTYPE html>
<html>
<head>
<title>Smart Water Meter</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }

 body {
    font-family: Arial, sans-serif;
    background: #001F54;      /* Navy Blue */
    color: white;
    text-align: center;
    padding-bottom: 40px;
}

  h1 {
    padding: 24px 0 8px;
    font-size: 1.6rem;
    letter-spacing: 0.08em;
    color: white;
}

  /* ---------- KPI CARDS (original style kept) ---------- */
  .cards-row {
    display: flex;
    justify-content: center;
    flex-wrap: wrap;
    gap: 16px;
    margin: 20px auto;
    max-width: 920px;
    padding: 0 12px;
  }

  .card {
    background: #123B80;
    border:1px solid #3C6DD5;
    flex: 1;
    min-width: 200px;
    max-width: 260px;
    padding: 20px;
    border-radius: 10px;
    box-shadow: 0 0 10px gray;
  }

  .card h2 {
    font-size: 1rem;
    color: #C8D8FF; margin-bottom: 10px; }

  .value {
    font-size: 36px;
    font-weight: bold;
    color: white; }

  .normal{
    background:#2ECC71;
    color:white;
}

.minor-monitor{
    background:#FFD54F;
    color:black;
}

.major-monitor{
    background:#FB8C00;
    color:white;
}

.minor{
    background:#EF6C00;
    color:white;
}

.major{
    background:#C62828;
    color:white;
}

  /* ---------- RESET BUTTON ---------- */
  .reset-btn {
    display: inline-block;
    margin: 4px auto 20px;
    padding: 10px 32px;
    background: #1a237e;
    color: white;
    border: none;
    border-radius: 8px;
    font-size: 0.95rem;
    font-weight: 600;
    letter-spacing: 0.06em;
    cursor: pointer;
    transition: background 0.2s;
  }
  .reset-btn:hover { background: #283593; }
  .reset-btn:active { background: #0d1457; }

  /* ---------- PANELS ---------- */
  .panel {
    background: #123B80;
    border:1px solid #3C6DD5;   
    border-radius: 10px;
    box-shadow: 0 0 10px rgba(0,0,0,0.12);
    padding: 20px 24px;
    margin: 0 auto 24px;
    max-width: 920px;
    text-align: left;
  }

  .panel-title {
    font-size: 0.8rem;
    font-weight: 700;
    letter-spacing: 0.12em;
    text-transform: uppercase;
    color: #7EC8FF;
    border-bottom: 2px solid #2E5AAC;
    padding-bottom: 10px;
    margin-bottom: 16px;
  }

  canvas { max-height: 260px; background:#0A2A66; }

  /* ---------- TABLE ---------- */
  .table-wrap {
    overflow-x: auto;
    max-height: 280px;
    overflow-y: auto;
  }

  table { width: 100%; border-collapse: collapse; font-size: 0.88rem; }

  thead th {
    position: sticky;
    top: 0;
    background: #e8eaf6;
    color: #1a237e;
    font-size: 0.75rem;
    letter-spacing: 0.08em;
    text-transform: uppercase;
    padding: 10px 14px;
    text-align: left;
  }

  tbody tr { border-bottom: 1px solid #234D96; }
  tbody tr:hover { background: #123C8C; }
  td { padding: 8px 14px; color: white  ; }

  .s-normal { color: green;  font-weight: 700; }
  .s-minor  { color: orange; font-weight: 700; }
  .s-major  { color: red;    font-weight: 700; }

  .dot {
    display: inline-block;
    width: 8px; height: 8px;
    border-radius: 50%;
    margin-right: 6px;
    vertical-align: middle;
  }
  .dot-normal { background: green; }
  .dot-minor  { background: orange; }
  .dot-major  { background: red; }

  .empty-row td { text-align: center; color: #aaa; padding: 24px; }
</style>
</head>
<body>

<h1>SMART WATER METER</h1>

<!-- KPI Cards -->
<div class="cards-row">
  <div class="card">
    <h2>Pulse Count</h2>
    <div id="pulse" class="value">0</div>
  </div>
  <div class="card">
    <h2>Volume</h2>
    <div id="volume" class="value">0 mL</div>
  </div>
  <div class="card">
    <h2>Status</h2>
    <div id="status" class="value normal">NORMAL</div>
  </div>
</div>

<!-- Reset Button -->
<button class="reset-btn" onclick="resetData()">&#8635; Reset Status</button>

<!-- Real-Time Chart -->
<div class="panel" style="max-width:920px; margin: 0 auto 24px;">
  <div class="panel-title">Real-Time Trend</div>
  <canvas id="trendChart"></canvas>
</div>

<!-- History Table -->
<div class="panel" style="max-width:920px; margin: 0 auto 24px;">
  <div class="panel-title">Reading History (latest first)</div>
  <div class="table-wrap">
    <table>
      <thead>
        <tr>
          <th>#</th>
          <th>Time</th>
          <th>Pulses</th>
          <th>Volume (mL)</th>
          <th>Status</th>
        </tr>
      </thead>
      <tbody id="historyBody">
        <tr class="empty-row"><td colspan="5">Waiting for data…</td></tr>
      </tbody>
    </table>
  </div>
</div>

<script>
// ---- Chart setup ----
const MAX_POINTS = 40;
const chartLabels = [];
const pulseBuf    = [];
const volBuf      = [];

const ctx = document.getElementById('trendChart').getContext('2d');
const chart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: chartLabels,
    datasets: [
      {
        label: 'Pulses',
        data: pulseBuf,
        borderColor: '#00E5FF',
        backgroundColor: 'rgba(0,229,255,0.15)',
        pointBackgroundColor: '#00E5FF',
        pointBorderColor: '#FFFFFF',
        borderWidth: 2,
        pointRadius: 3,
        tension: 0.35,
        yAxisID: 'yPulse'
      },
      {
        label: 'Volume (mL)',
        data: volBuf,
        borderColor: '#2e7d32',
        backgroundColor: 'rgba(46,125,50,0.07)',
        borderWidth: 2,
        pointRadius: 3,
        tension: 0.35,
        yAxisID: 'yVol'
      }
    ]
  },
  options: {
    responsive: true,
    animation: { duration: 300 },
    interaction: { mode: 'index', intersect: false },
    plugins: {
      legend: { labels: { color: '#FFFFFF',
        font: {
            size: 13,
            weight: 'bold' } } }
    },
    scales: {
      x: {
        ticks: { maxTicksLimit: 10, color: '#FFFFFF' },
        grid:  { color: 'rgba(255,255,255,0.15)' }
      },
      yPulse: {
        position: 'left',
        title: { display: true, text: 'Pulses', color: '#00E5FF', font: {
        size: 15,
        weight: 'bold'
    } },
        ticks: { color: '#FFFFFF' },
        grid:  { color: 'rgba(255,255,255,0.15)' }
      },
      yVol: {
        position: 'right',
        title: { display: true, text: 'Volume (mL)', color: '#00FF7F', font: {
        size: 15,
        weight: 'bold'
    } },
        ticks: { color: '#FFFFFF' },
        grid:  { drawOnChartArea: false }
      }
    }
  }
});

// ---- Helpers ----
function statusClass(s) {
  if (s === 'NORMAL')     return 's-normal';
  if (s === 'MINOR LEAK') return 's-minor';
  return 's-major';
}
function dotClass(s) {
  if (s === 'NORMAL')     return 'dot-normal';
  if (s === 'MINOR LEAK') return 'dot-minor';
  return 'dot-major';
}

// ---- Reset ----
function resetData() {
  fetch('/reset', { method: 'POST' })
    .then(() => {
      // Only reset the status badge back to NORMAL
      const sb = document.getElementById('status');
      sb.textContent = 'NORMAL';
      sb.className   = 'value normal';
    });
}

// ---- Poll ----
let lastHistoryLen = -1;

function updateData() {
  // KPI cards  (original logic)
  fetch('/data')
    .then(r => r.json())
    .then(d => {
      document.getElementById('pulse').innerHTML  = d.pulses;
      document.getElementById('volume').innerHTML = d.volume + ' mL';
      let statusBox = document.getElementById("status");

statusBox.innerHTML = d.status;

statusBox.className = "value";

switch(d.status)
{
    case "NORMAL":
        statusBox.classList.add("normal");
        break;

    case "MINOR MONITORING":
        statusBox.classList.add("minor-monitor");
        break;

    case "MAJOR MONITORING":
        statusBox.classList.add("major-monitor");
        break;

    case "MINOR LEAK":
        statusBox.classList.add("minor");
        break;

    case "MAJOR LEAK":
        statusBox.classList.add("major");
        break;

    default:
        statusBox.classList.add("normal");
}
    });

  // Chart + table
  fetch('/history')
    .then(r => r.json())
    .then(hist => {
      if (hist.length === lastHistoryLen) return;
      lastHistoryLen = hist.length;

      // Add only the newest point to the chart
      const latest = hist[hist.length - 1];
      if (latest) {
        chartLabels.push(latest.time);
        pulseBuf.push(parseFloat(latest.pulses));
        volBuf.push(parseFloat(latest.volume));
        if (chartLabels.length > MAX_POINTS) {
          chartLabels.shift(); pulseBuf.shift(); volBuf.shift();
        }
        chart.update();
      }

      // Rebuild table newest-first
      const tbody = document.getElementById('historyBody');
      if (hist.length === 0) {
        tbody.innerHTML = '<tr class="empty-row"><td colspan="5">Reset — waiting for new data…</td></tr>';
        return;
      }
      const rows = [...hist].reverse();
      tbody.innerHTML = rows.map((r, i) => `
        <tr>
          <td style="color:#aaa">${hist.length - i}</td>
          <td>${r.time}</td>
          <td>${r.pulses}</td>
          <td>${r.volume}</td>
          <td class="${statusClass(r.status)}">
            <span class="dot ${dotClass(r.status)}"></span>${r.status}
          </td>
        </tr>`).join('');
    });
}

setInterval(updateData, 1000);
updateData();
</script>
</body>
</html>
"""

# =========================
# ROUTES
# =========================
@app.route("/")
def home():
    return render_template_string(HTML)

@app.route("/data")
def get_data():
    return jsonify(data)

@app.route("/history")
def get_history():
    return jsonify(list(history))

@app.route("/reset", methods=["POST"])
def reset():

    global ser

    print("Reset button pressed")

    try:
        if ser is not None and ser.is_open:
            print("Sending R")
            ser.write(b'R')
            ser.flush()
            print("R Sent")
        else:
            print("Serial port not open")

    except Exception as e:
        print(e)

    return jsonify({"ok": True})
    """Clear history and reset live data to zero."""
    data["status"] = "NORMAL"
    return jsonify({"ok": True})

# =========================
# MAIN
# =========================
if __name__ == "__main__":
    threading.Thread(
        target=serial_reader,
        daemon=True
    ).start()
    app.run(
        host="0.0.0.0",
        port=5000,
        debug=False
    )
