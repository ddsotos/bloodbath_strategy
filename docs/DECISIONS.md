# Design decisions and open experiments

Keep **confirmed requirements**, **experimental hypotheses**, and **unknown rule details** separate. Refer to the rule checklist before implementing real gameplay.

## Confirmed project decisions
- Two-player research first; strongest practical policy, not real-time speed, is the goal.
- Official outcome is frozen at its first legal terminal event; shadow continuation proceeds both players to a common, explicitly chosen horizon.
- Each player's first death has an immutable timestamp (deck cycle, position within cycle, absolute progression event), including when a player later recovers.
- After first death, choose actions using a single specified, permanently fixed policy. Do not switch back to a smart agent after recovery.
- Raw own/opponent final shadow scores and death timestamps are both important; official wins must remain separately available.
- Paired add/omit shared-deck interventions should estimate changes to both players' downstream outcomes.
- The lead agent owns unresolved rule/algorithm decisions; subagents provide evidence, tests and reproducible reports.

## High-priority decisions **not yet settled**
1. **Common horizon:** determine the last meaningful deck cycle/event from authoritative rules; if there is no single rule-defined endpoint, choose an experimental finite horizon and sensitivity-test several horizons. Log censored/incomplete continuations.
2. **What counts as first death:** define zero, negative population, inability to pay a population loss, and simultaneous deaths from precise official wording. Note that a signed negative figure may be a shadow accounting variable, *not legal population*.
3. **Fixed post-death policy:** decide deterministic priorities and tie-breaking *after* checking what actions remain possible; include a no-op fallback and a version ID. Evaluate whether conclusions change under reasonable alternate fixed policies.
4. **Deaths vs shadow score:** do not choose a constant penalty without adversarial tests. Preserve vector data; compare death-first lexicographic objectives, bounded shadow score with death penalty, and smooth penalties. Earlier death must always produce larger mortality penalty under otherwise identical conditions. A huge post-death fortune must not silently erase an early death.
5. **Both players' timeline after official game over:** record each first death separately, continue each under its own policy, and handle no-choice/undefined actions via a documented shadow adapter, not made-up official gameplay.
6. **Training label vs objective:** raw score helps distinguish outcomes against stronger opponents, but official win probability remains a separately measured external metric. Compare multiple reward objectives at equal compute and across held-out seeds.
7. **Search and information leakage:** prevent look-ahead from determinized decks and simultaneous opponent action commitments; validate on tiny exact-solvable toy games.
8. **Counterfactual deck insertion:** paired stochastic coupling must respect changes in deck composition and shuffle timing. Ensure the opponent chooses with its own observation per branch.
9. **Evaluation budgets:** distinguish self-play training cost from decision inference time; report wall-time, total simulations, hardware, seed family, and rule/model/adapter hashes. Keep independent held-out evaluation sets.
10. **Rule/card provenance and IP:** authoritative texts must be cited/versioned; avoid committing wholesale copyrighted card/rule text unless distribution rights permit. Record compact derived data and provenance appropriately.

## Questions worth answering by experiment rather than assumption
- How strongly do self shadow score and first-death timing correlate with official victory?
- Does fixed post-death policy change the ranking of first-Work draw vs alternatives?
- Do card-addition counterfactuals materially improve the estimate of opponent score?
- Does more search improve play under each chosen reward objective?
- Does a learned value model outperform pure rollouts at equal total compute?

## Next milestone gate
Do not start strong search training until the rule subset is verified, observation leaks are tested, and official/shadow continuation passes mortality-invariant tests. In the first PR, code only the portable skeleton and toy fixtures if full card rules remain unverified.
