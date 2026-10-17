from fastapi import FastAPI, HTTPException

from app.models import TelemetryIn, TelemetryRecord
from app.store import TelemetryStore

app = FastAPI(
    title="Edge Telemetry Receiver",
    description="Cloud endpoint for the Linux edge telemetry daemon",
    version="1.0.0",
)

store = TelemetryStore()


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}


@app.post("/api/v1/telemetry", response_model=TelemetryRecord)
def ingest_telemetry(payload: TelemetryIn) -> TelemetryRecord:
    if payload.mem_available_kb > payload.mem_total_kb:
        raise HTTPException(status_code=422, detail="mem_available_kb exceeds mem_total_kb")
    return store.ingest(payload)


@app.get("/api/v1/telemetry/latest", response_model=TelemetryRecord)
def latest_telemetry() -> TelemetryRecord:
    record = store.latest()
    if record is None:
        raise HTTPException(status_code=404, detail="no telemetry received yet")
    return record


@app.get("/api/v1/telemetry/latest/{host_id}", response_model=TelemetryRecord)
def latest_for_host(host_id: str) -> TelemetryRecord:
    record = store.latest_for_host(host_id)
    if record is None:
        raise HTTPException(status_code=404, detail=f"no telemetry for host {host_id}")
    return record


@app.get("/api/v1/stats")
def stats() -> dict[str, int]:
    return {"messages_received": store.count()}
