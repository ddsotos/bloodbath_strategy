"""Check the p11 Budget Cuts fields in a synthetic conditional trace."""
import json
import subprocess
import sys

record = json.loads(subprocess.check_output([sys.argv[1], "--budget-cuts-json"], text=True))
assert record["schema_version"] == 13 and record["full_game"] is False
assert len(record["commands"]) == len(record["checkpoints"]) == 1
checkpoint = record["checkpoints"][0]
assert checkpoint["trace_assessment"]["status"] == "in_progress"
assert checkpoint["budget_cuts"] == {"money_loss": [2, 1]}
assert [p["money"] for p in checkpoint["colonies"]["players"]] == [0, 0]
assert record["final"] == checkpoint
print("Budget Cuts JSON audit passed")
