import json
import subprocess
import sys

trace = json.loads(subprocess.check_output([sys.argv[1], "--json"], text=True))
assert trace["schema_version"] == 12 and trace["full_game"] is False
expected = dict(status="blocked_rule", reason="butler_box_selector_unverified",
    terminal_resolved=False, has_conditional_outcome=False)
assert trace["trace_assessment"] == expected
final = trace["final"]
assert final == trace["checkpoints"][-1] and final["trace_assessment"] == expected
assert final["phase"] == 13 and final["robot"]["completed"] is False
assert final["pending_robot"] == 8 and final["progress"]["active"] == 100
assert final["progress"]["clock"] == dict(absolute_event=1, cycle=0, position=1, decision=0)
assert [p["base_people"] for p in final["colonies"]["players"]] == [6, 6]
for before, after in zip(trace["initial"]["colonies"]["players"], final["colonies"]["players"]):
    assert before["boxes"] == after["boxes"]
assert final["conditional_official_outcome"] is None
assert final["first_deaths"] == [None, None] and trace["shadow"] is None

for mode in ([], ["--paperwork"], ["--robots"], ["--tableau"]):
    resolved = json.loads(subprocess.check_output([sys.argv[2], *mode], text=True))
    assessment = resolved["trace_assessment"]
    assert assessment == dict(status="conditional_terminal", reason=None,
        terminal_resolved=True, has_conditional_outcome=True)
    assert assessment == resolved["final"]["trace_assessment"]
    assert resolved["full_game"] is False
    for point in resolved["checkpoints"]:
        view = point["trace_assessment"]
        assert view["has_conditional_outcome"] == (point["conditional_official_outcome"] is not None)
        if point["phase"] in (1, 8, 9):
            assert view["status"] == "awaiting_input" and view["terminal_resolved"] is False
print("Trace assessment schema-12 JSON audit passed")
