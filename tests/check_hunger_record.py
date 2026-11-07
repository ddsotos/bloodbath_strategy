import json
import subprocess
import sys

trace = json.loads(subprocess.check_output([sys.argv[1], "--json"], text=True))
assert trace["schema_version"] == 10 and trace["full_game"] is False
assert trace["policy_version"] == "caller-commands-v10"
assert [c["kind"] for c in trace["commands"]] == [0, 0]
initial = trace["initial"]["colonies"]
first, final = trace["checkpoints"]
assert first["hunger"] == dict(required_food=[2, 1], spent_food=[1, 1],
    population_loss=[1, 0], population_gain=[0, 2])
assert final == trace["final"]
assert final["hunger"] == dict(required_food=[2, 1], spent_food=[0, 1],
    population_loss=[2, 0], population_gain=[0, 2])
assert final["phase"] == 0 and final["progress"]["active"] is None
assert final["progress"]["clock"] == dict(absolute_event=2, cycle=0, position=2, decision=0)
players = final["colonies"]["players"]
assert [p["base_people"] for p in players] == [7, 14]
assert [p["total_people"] for p in players] == [17, 19]
assert [p["food"] for p in players] == [0, 2]
for p, before in zip(players, initial["players"]):
    for field in ("hand", "buildings", "boxes", "money"):
        assert p[field] == before[field]
for field in ("building_deck", "building_discard", "building_shuffle_index"):
    assert final["colonies"][field] == initial[field]
assert final["first_deaths"] == [None, None]
assert final["conditional_official_outcome"] is None and trace["shadow"] is None
print("Hunger schema-10 JSON audit passed")
