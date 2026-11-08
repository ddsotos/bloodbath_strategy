"""Independently inspect a synthetic Flag BUILD trace."""
import json
import subprocess
import sys

record = json.loads(subprocess.check_output([sys.argv[1], "--json"], text=True))
assert record["schema_version"] == 13 and record["full_game"] is False
assert len(record["commands"]) == len(record["checkpoints"]) == 4
final = record["final"]
player = final["colonies"]["players"][0]
assert player["money"] == 10 and player["food"] == 4
assert player["buildings"] == [0] and player["total_people"] == 30
assert final["tableau_work"]["effects"][:3] == [
    {"player": 0, "kind": 0, "card": 0, "amount": 0},
    {"player": 0, "kind": 1, "card": 0, "amount": 0},
    {"player": 0, "kind": 7, "card": 0, "amount": 6},
]
print("Flag JSON audit passed")
