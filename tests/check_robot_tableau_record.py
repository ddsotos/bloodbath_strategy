import json
import subprocess
import sys

trace = json.loads(subprocess.check_output([sys.argv[1], "--json"], text=True))
assert trace["schema_version"] == 15 and trace["full_game"] is False
assert trace["policy_version"] == "caller-commands-v15"
assert [c["kind"] for c in trace["commands"]] == [0, 0]
initial = trace["initial"]["colonies"]
for point, effect, amount, food in [(trace["checkpoints"][0], 9, 4, [1, 2]),
                                    (trace["checkpoints"][1], 10, 6, [0, 0])]:
    audit = point["robot"]
    assert audit["effect"] == effect
    assert audit["losses"] == [dict(requested_loss=amount, paid=amount, unpaid_shortfall=0, lost=[])] * 2
    assert audit["food_loss"] == food and audit["money_gain"] == [0, 0]
    assert point["phase"] == 0 and point["progress"]["active"] is None
    assert point["first_deaths"] == [None, None]
    assert point["conditional_official_outcome"] is None
    for p, before in zip(point["colonies"]["players"], initial["players"]):
        for field in ("buildings", "boxes", "hand", "money"):
            assert p[field] == before[field]
    for field in ("building_deck", "building_discard", "building_shuffle_index"):
        assert point["colonies"][field] == initial[field]
final = trace["final"]
assert final == trace["checkpoints"][-1]
assert final["progress"]["clock"] == dict(absolute_event=2, cycle=0, position=2, decision=0)
players = final["colonies"]["players"]
assert [p["base_people"] for p in players] == [0, 1]
assert [p["total_people"] for p in players] == [5, 6]
assert [p["food"] for p in players] == [0, 2]
assert trace["shadow"] is None
print("Robot Tableau schema-14 JSON audit passed")
