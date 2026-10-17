from __future__ import annotations

import threading
import time
from collections import deque

from app.models import TelemetryIn, TelemetryRecord

_MAX_HISTORY = 500


class TelemetryStore:
    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._latest: TelemetryRecord | None = None
        self._history: deque[TelemetryRecord] = deque(maxlen=_MAX_HISTORY)
        self._by_host: dict[str, TelemetryRecord] = {}

    def ingest(self, payload: TelemetryIn) -> TelemetryRecord:
        record = TelemetryRecord(
            **payload.model_dump(),
            received_at_unix=int(time.time()),
        )
        with self._lock:
            self._latest = record
            self._history.append(record)
            self._by_host[record.host_id] = record
        return record

    def latest(self) -> TelemetryRecord | None:
        with self._lock:
            return self._latest

    def latest_for_host(self, host_id: str) -> TelemetryRecord | None:
        with self._lock:
            return self._by_host.get(host_id)

    def count(self) -> int:
        with self._lock:
            return len(self._history)
