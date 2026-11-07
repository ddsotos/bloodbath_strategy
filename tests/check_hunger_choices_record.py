import json
import subprocess
import sys

trace = json.loads(subprocess.check_output([sys.argv[1], "--json"], text=True))
assert trace["schema_version"] == 11 and trace["full_game"] is False
assert trace["policy_version"] == "caller-commands-v11"
assert [c["kind"] for c in trace["commands"]] == [0, 7, 7, 9]
points = trace["checkpoints"]
amounts = dict(required_food=[2, 1], spent_food=[0, 0], population_loss=[2, 1], population_gain=[0, 0])
for point in points[:3]:
    assert point["phase"] == 15 and point["pending_hunger"] == amounts
    assert point["hunger"] is None and point["hunger_losses"] is None
    assert point["colonies"] == trace["initial"]["colonies"]
assert points[1]["loss_commitments"] == [[1], None]
assert points[2]["loss_commitments"] == [[1], [4]]
final = trace["final"]
assert final == points[-1] and final["phase"] == 0
assert final["hunger"] == amounts and final["pending_hunger"] is None
assert final["loss_commitments"] == [None, None]
assert final["progress"]["active"] is None
assert final["progress"]["clock"] == dict(absolute_event=1, cycle=0, position=1, decision=1)
assert final["colonies"]["building_discard"] == [4, 1]
players = final["colonies"]["players"]
assert [p["base_people"] for p in players] == [3, 4]
assert [p["total_people"] for p in players] == [8, 4]
assert [p["food"] for p in players] == [0, 0]
assert [p["boxes"] for p in players] == [[[0, 2]], []]
audit = final["hunger_losses"]
assert audit["resolution_order"] == [1, 0]
for p, card, boxes, amount in [(0, 1, 7, 2), (1, 4, 3, 1)]:
    assert audit["losses"][p] == dict(requested_loss=amount, paid=amount, unpaid_shortfall=0,
        lost=[dict(card=card, transferred_people=5, discarded_boxes=boxes)])
assert final["first_deaths"] == [None, None]
assert final["conditional_official_outcome"] is None and trace["shadow"] is None
print("Hunger choice schema-11 JSON audit passed")
