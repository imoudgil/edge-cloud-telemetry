from fastapi.testclient import TestClient

from app.main import app

client = TestClient(app)

SAMPLE = {
    "host_id": "test-edge",
    "timestamp_unix": 1710000000,
    "cpu_usage_percent": 15.5,
    "mem_total_kb": 16000000,
    "mem_available_kb": 8000000,
    "mem_used_percent": 50.0,
    "temperature_celsius": 41.2,
}


def test_health() -> None:
    r = client.get("/health")
    assert r.status_code == 200
    assert r.json()["status"] == "ok"


def test_ingest_and_latest() -> None:
    r = client.post("/api/v1/telemetry", json=SAMPLE)
    assert r.status_code == 200
    body = r.json()
    assert body["host_id"] == SAMPLE["host_id"]
    assert "received_at_unix" in body

    r2 = client.get("/api/v1/telemetry/latest")
    assert r2.status_code == 200
    assert r2.json()["host_id"] == SAMPLE["host_id"]

    r3 = client.get("/api/v1/telemetry/latest/test-edge")
    assert r3.status_code == 200


def test_rejects_invalid_mem() -> None:
    bad = {**SAMPLE, "mem_available_kb": SAMPLE["mem_total_kb"] + 1}
    r = client.post("/api/v1/telemetry", json=bad)
    assert r.status_code == 422
