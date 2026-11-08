import json
import subprocess
import sys

trace = json.loads(subprocess.check_output([sys.argv[1], "--json"], text=True))
assert trace["schema_version"] == 14 and trace["full_game"] is False
assert trace["policy_version"] == "caller-commands-v14"
assert [c["kind"] for c in trace["commands"]] == [0, 7, 7, 10]
points = trace["checkpoints"]
for point in points[:3]:
    assert point["phase"] == 16 and point["pending_robot"] == 9 and point["robot"] is None
    assert point["colonies"] == trace["initial"]["colonies"]
assert points[1]["loss_commitments"] == [[1], None]
assert points[2]["loss_commitments"] == [[1], [5]]
final = trace["final"]
assert final == points[-1] and final["phase"] == 0 and final["pending_robot"] is None
assert final["loss_commitments"] == [None, None]
assert final["progress"]["active"] is None
assert final["progress"]["clock"] == dict(absolute_event=1, cycle=0, position=1, decision=1)
assert final["colonies"]["building_discard"] == [5, 1]
players = final["colonies"]["players"]
assert [p["base_people"] for p in players] == [1, 1]
assert [p["total_people"] for p in players] == [6, 6]
assert [p["food"] for p in players] == [0, 2]
assert [p["buildings"] for p in players] == [[0], [4]]
assert [p["boxes"] for p in players] == [[[0, 2]], [[4, 3]]]
audit = final["robot"]
assert audit["effect"] == 9 and audit["resolution_order"] == [1, 0]
assert audit["food_loss"] == [1, 2] and audit["money_gain"] == [0, 0]
for p, card, boxes in [(0, 1, 7), (1, 5, 8)]:
    assert audit["losses"][p] == dict(requested_loss=4, paid=4, unpaid_shortfall=0,
        lost=[dict(card=card, transferred_people=5, discarded_boxes=boxes)])
assert final["first_deaths"] == [None, None]
assert final["conditional_official_outcome"] is None and trace["shadow"] is None
print("Robot choice schema-14 JSON audit passed")
