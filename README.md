# Edge-to-Cloud Linux Telemetry Daemon

A native Linux edge agent written in pure C that streams hardware telemetry to a containerized Python backend.

Three sampler threads read **CPU**, **memory**, and **temperature** directly from the Linux `/proc` and `/sys` virtual filesystems, a sender thread POSTs them as JSON, and a **FastAPI** service running in **Docker** ingests and serves the data. The agent ships as a native **systemd** service, and **GitHub Actions** builds the C binary and lints/tests the Python backend on every push.

## High-level architecture

```
Linux edge host                          Cloud (Docker)
+---------------------------+            +---------------------------+
|  edge-telemetry (binary)  |  HTTP POST |  uvicorn + FastAPI       |
|                           |  JSON      |  port 8000               |
|  cpu_sampler_thread       | ---------> |  POST /api/v1/telemetry  |
|  mem_sampler_thread       |            |  in-memory TelemetryStore|
|  temp_sampler_thread      |            +---------------------------+
|  sender_thread            |
|  shared telemetry_state   |
|  (mutex-protected)        |
+---------------------------+
        ^
        | reads
   /proc/stat, /proc/meminfo, /sys/class/thermal/*
```

Three sampler threads continuously update a shared, mutex-protected snapshot. A fourth sender thread serializes that snapshot to JSON and HTTP POSTs it to the cloud endpoint every `TELEMETRY_INTERVAL_SEC` seconds.

## How to run

### 1. Start the cloud receiver (Docker)

```bash
docker compose up --build
# health check
curl http://localhost:8000/health
```

The API is now live on `http://localhost:8000` (interactive docs at `/docs`).

### 2. Build and run the edge agent (Linux)

```bash
cd edge-agent
make
sudo make install                       # installs binary + systemd unit
sudo systemctl enable --now edge-telemetry.service
journalctl -u edge-telemetry -f         # watch it stream
```

> The C agent is **Linux-only** (it reads `/proc` and `/sys`). On macOS/Windows, build and run it inside a Linux container or VM.

### 3. Verify data is flowing

```bash
curl http://localhost:8000/api/v1/telemetry/latest
```

## Configuration (edge agent)

Set via environment variables (in the systemd unit or shell):

| Variable | Default | Description |
|----------|---------|-------------|
| `TELEMETRY_ENDPOINT` | `http://127.0.0.1:8000/api/v1/telemetry` | Cloud POST URL |
| `TELEMETRY_INTERVAL_SEC` | `5` | Seconds between sends |
| `TELEMETRY_HOST_ID` | hostname | Logical device id in the JSON payload |

## API endpoints

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/health` | Liveness check |
| `POST` | `/api/v1/telemetry` | Ingest a telemetry payload |
| `GET` | `/api/v1/telemetry/latest` | Most recent payload (any host) |
| `GET` | `/api/v1/telemetry/latest/{host_id}` | Most recent payload for one host |
| `GET` | `/api/v1/stats` | Count of messages received |

## JSON payload

```json
{
  "host_id": "edge-node-1",
  "timestamp_unix": 1710000000,
  "cpu_usage_percent": 12.34,
  "mem_total_kb": 16384000,
  "mem_available_kb": 8192000,
  "mem_used_percent": 50.0,
  "temperature_celsius": 42.5
}
```

`temperature_celsius` is `-1.0` when no thermal zone is available (common in VMs).

## Development

```bash
# Python backend
cd cloud-receiver
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements-dev.txt
ruff check app tests
pytest -q

# C edge agent (Linux)
cd edge-agent && make clean all
```

## Project layout

```
edge-cloud-telemetry/
├── edge-agent/        # C daemon: /proc + /sys samplers, pthreads, systemd
├── cloud-receiver/    # FastAPI backend + Dockerfile + tests
├── docker-compose.yml # builds/runs the receiver on :8000
└── .github/workflows/ # CI: builds C, lints + tests Python
```

## CI

GitHub Actions (`.github/workflows/ci.yml`) runs on every push/PR:

- **edge-agent:** builds the C daemon with `-Werror`
- **cloud-receiver:** `ruff` lint, `pytest`, and a Docker build
