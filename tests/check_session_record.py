"""Parse the standalone conditional trace; these are synthetic scenario inputs."""
import json
import subprocess
import sys

record = json.loads(subprocess.check_output([sys.argv[1]], text=True))
assert record["schema_version"] == 3
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
assert final["paperwork"] is None
assert final["discard_commitments"] == [None, None]

robots = json.loads(subprocess.check_output([sys.argv[1], "--robots"], text=True))
assert robots["schema_version"] == 3 and robots["full_game"] is False
assert len(robots["commands"]) == len(robots["checkpoints"]) == 7
assert robots["initial"]["config"]["ordered_robots"] == [108]
for point in robots["checkpoints"]:
    colony = point["colonies"]
    ids = colony["building_deck"] + colony["building_discard"]
    for player in colony["players"]:
        ids += player["hand"] + player["buildings"]
    assert sorted(ids) == list(range(90))
    progress = point["progress"]
    ids = progress["deck"] + progress["discard"]
    if progress["active"] is not None:
        ids += [progress["active"]]
    ids += robots["initial"]["config"]["ordered_robots"][point["next_robot"]:]
    assert sorted(ids) == [100, 104, 107, 108]
glitch = robots["checkpoints"][4]
assert glitch["next_robot"] == 1 and glitch["progress"]["deck"][-1] == 108
assert [p["people"] for p in glitch["colonies"]["players"]] == [30, 29]
assert robots["final"]["next_robot"] == 1
assert robots["final"]["conditional_official_outcome"]["scores"] == [26, 25]
assert [p["food"] for p in robots["final"]["colonies"]["players"]] == [2, 6]
assert robots["final"]["progress"]["clock"] == {"absolute_event": 4, "cycle": 0, "position": 4, "decision": 1}
assert robots["final"]["first_deaths"] == [None, None]

paperwork = json.loads(subprocess.check_output([sys.argv[1], "--paperwork"], text=True))
assert paperwork["schema_version"] == 3 and paperwork["full_game"] is False
assert len(paperwork["commands"]) == len(paperwork["checkpoints"]) == 10
for point in paperwork["checkpoints"]:
    colony = point["colonies"]
    ids = colony["building_deck"] + colony["building_discard"]
    for player in colony["players"]:
        ids += player["hand"] + player["buildings"]
    assert sorted(ids) == list(range(90))
prepared = paperwork["checkpoints"][5]
assert prepared["paperwork"]["drawn"] == [[86], [87]]
assert prepared["paperwork"]["draw_order"] == [1, 0]
assert prepared["paperwork"]["completed"] is False
assert paperwork["checkpoints"][6]["discard_commitments"] == [[0, 86], None]
final = paperwork["final"]
assert final["conditional_official_outcome"]["scores"] == [30, 29]
assert final["progress"]["clock"]["absolute_event"] == 3
assert final["progress"]["clock"]["decision"] == 2
assert final["paperwork"]["discarded"] == [[0, 86], [4, 87]]
assert final["paperwork"]["discard_order"] == [1, 0]
assert final["paperwork"]["completed"] is True
assert final["colonies"]["building_discard"] == [4, 87, 0, 86]
assert final["discard_commitments"] == [None, None]
print("conditional session JSON independently parsed and conserved")
