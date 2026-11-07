import json
import subprocess
import sys

record = json.loads(subprocess.check_output([sys.argv[1], "--json"], text=True))
assert record["schema_version"] == 6 and record["full_game"] is False
assert record["policy_version"] == "caller-commands-v6"
assert [c["kind"] for c in record["commands"]] == [0, 7, 7, 8]
points = record["checkpoints"]
assert points[0]["phase"] == 14 and points[0]["accident"] is None
assert points[1]["loss_commitments"] == [[1], None]
assert points[2]["loss_commitments"] == [[1], [4]]
final = record["final"]
assert final == points[-1] and final["phase"] == 0
assert final["loss_commitments"] == [None, None]
assert final["progress"]["clock"] == dict(absolute_event=1, cycle=0, position=1, decision=1)
assert final["progress"]["active"] is None
assert final["colonies"]["building_discard"] == [4, 1]
players = final["colonies"]["players"]
assert [p["base_people"] for p in players] == [3, 3]
assert [p["total_people"] for p in players] == [8, 3]
assert [p["boxes"] for p in players] == [[[0, 2]], []]
audit = final["accident"]
assert audit["resolution_order"] == [1, 0]
for p, card, boxes in [(0, 1, 7), (1, 4, 3)]:
    assert audit["losses"][p] == dict(requested_loss=3, paid=3, unpaid_shortfall=0,
        lost=[dict(card=card, transferred_people=5, discarded_boxes=boxes)])
assert final["first_deaths"] == [None, None]
assert final["conditional_official_outcome"] is None
assert record["shadow"] is None
print("Accident schema-6 JSON audit passed")
