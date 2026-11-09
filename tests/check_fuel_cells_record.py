"""Independently audit Fuel Cells privacy and replay JSON."""
import json
import subprocess
import sys

record = json.loads(subprocess.check_output([sys.argv[1], "--json"], text=True))
assert record["schema_version"] == 15 and record["full_game"] is False
assert record["policy_version"] == "caller-commands-v15"
assert record["commands"][1]["fuel_cells_spend"] is True
assert record["commands"][4]["cards"] == [89]
pre_reveal = record["checkpoints"][1]
assert pre_reveal["work_choices"][0]["spend_fuel_cells"] is True
assert pre_reveal["work_choices"][1] is None
pending = record["checkpoints"][3]
assert pending["phase"] != record["final"]["phase"]  # Pending input uses a distinct phase.
assert pending["pending_fuel_cells_discard_player"] == 0
assert pending["tableau_work"]["choices"][0]["spend_fuel_cells"] is True
drawn = pending["tableau_work"]["drawn"][0]
assert len(drawn) == 1 and drawn[0] == 89
assert pending["tableau_work"]["fuel_cells_discarded"] == [None, None]
final = record["final"]
assert final["pending_fuel_cells_discard_player"] is None
assert final["tableau_work"]["fuel_cells_discarded"] == [drawn[0], None]
assert final["colonies"]["building_discard"][-1] == drawn[0]
assert final["colonies"]["players"][0]["boxes"] == [[0, 1]]
assert final["colonies"]["players"][0]["money"] == 8
assert [effect["kind"] for effect in final["tableau_work"]["effects"]] == [4, 4, 9, 7, 6, 5]
print("Fuel Cells schema-15 JSON audit passed")
