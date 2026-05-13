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


@dataclass(frozen=True)
class BaselineEvent:
    peak: int
    width: int
    area: int
    activate_value: int
    state: int

    @classmethod
    def from_tuple(cls, values):
        return cls(*values)
