from dataclasses import dataclass


@dataclass(frozen=True)
class BaselineFrame:
    result: float
    baseline: float
    threshold: float
    sample: float
    state: float

    @classmethod
    def from_tuple(cls, values):
        return cls(*values)
