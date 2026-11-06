"""Independently parse the C++ toy experiment record and its embedded replay."""
import json
import subprocess
import sys

record = json.loads(subprocess.check_output([sys.argv[1], "--json"], text=True))
assert record["schema_version"] == 1
assert record["fixture"] == "toy-only"
assert record["seed"] == 42
assert record["config"] == {"initial_population": [2, 2], "horizon": 12}
assert record["shadow"]["horizon"]["absolute_event"] == 12
assert record["official"]["terminal"]["absolute_event"] <= 12
assert len(record["official"]["scores"]) == len(record["shadow"]["scores"]) == 2
assert len(record["shadow"]["first_deaths"]) == 2
for player, death in enumerate(record["shadow"]["first_deaths"]):
    if death is not None:
        assert death["player"] == player
        assert 1 <= death["clock"]["absolute_event"] <= 12
        assert death["deficit"] >= 0
assert len(record["replay"].splitlines()) == 13
print("experiment JSON parsed independently")
