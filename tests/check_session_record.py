"""Parse the standalone conditional trace; these are synthetic scenario inputs."""
import json
import subprocess
import sys

record = json.loads(subprocess.check_output([sys.argv[1]], text=True))
assert record["schema_version"] == 1
assert record["full_game"] is False
assert record["scope"] == "conditional-no-buildings-no-modifiers"
for name in ("session_hash", "base_rules_hash", "progression_population_hash"):
    assert len(record[name]) == 64 and int(record[name], 16) >= 0
assert record["shadow"] is None
assert len(record["commands"]) == len(record["checkpoints"]) == 9
initial = record["initial"]
assert [p["people"] for p in initial["colonies"]["players"]] == [30, 29]
assert initial["config"]["seed"] == 42
for point in record["checkpoints"]:
    colony = point["colonies"]
    ids = colony["building_deck"] + colony["building_discard"]
    for player in colony["players"]:
        ids += player["hand"] + player["buildings"]
    assert sorted(ids) == list(range(90))
    progress = point["progress"]
    ids = progress["deck"] + progress["discard"]
    if progress["active"] is not None:
        ids += [progress["active"]]
    ids += initial["config"]["ordered_events"][point["next_event"]:]
    assert sorted(ids) == list(range(100, 106))
final = record["final"]
assert final == record["checkpoints"][-1]
assert final["progress"]["clock"] == {"absolute_event": 6, "cycle": 0, "position": 6, "decision": 1}
assert final["conditional_official_outcome"]["scores"] == [31, 30]
assert final["conditional_official_outcome"]["winner"] == 0
assert final["conditional_official_outcome"]["cause"] == "Instruction Manual"
assert final["first_deaths"] == [None, None]
assert final["next_event"] == 1
assert len(final["colonies"]["players"][0]["hand"]) == 6
assert final["colonies"]["players"][1]["food"] == 8
print("conditional session JSON independently parsed and conserved")
