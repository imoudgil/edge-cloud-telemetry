from pydantic import BaseModel, Field


class TelemetryIn(BaseModel):
    host_id: str = Field(min_length=1, max_length=256)
    timestamp_unix: int
    cpu_usage_percent: float = Field(ge=0.0, le=100.0)
    mem_total_kb: int = Field(ge=0)
    mem_available_kb: int = Field(ge=0)
    mem_used_percent: float = Field(ge=0.0, le=100.0)
    temperature_celsius: float = Field(description="-1 when sensor unavailable")


class TelemetryRecord(TelemetryIn):
    received_at_unix: int
