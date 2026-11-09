"""Check the hidden Potato Farm payment commitment in replay JSON."""
import json
import subprocess
import sys

record = json.loads(subprocess.check_output([sys.argv[1], "--json"], text=True))
assert record["schema_version"] == 15 and record["full_game"] is False
assert record["commands"][1]["food_payment"] == 1
assert record["checkpoints"][1]["work_choices"][0]["food_payment"] == 1
assert record["checkpoints"][1]["work_choices"][1] is None
final = record["final"]
assert final["tableau_work"]["choices"][0]["food_payment"] == 1
assert final["tableau_work"]["effects"][:3] == [
    {"player": 0, "kind": 0, "card": 1, "amount": 1},
    {"player": 0, "kind": 8, "card": 1, "amount": 1},
    {"player": 0, "kind": 1, "card": 1, "amount": 5},
]
assert [p["money"] for p in final["colonies"]["players"]] == [0, 8]
assert [p["food"] for p in final["colonies"]["players"]] == [7, 4]
print("Potato Farm payment JSON audit passed")
