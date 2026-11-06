# Research design

## Questions

1. Across opening observations, is **Research/draw on the first Work** stronger than its best alternative?
2. Which initial hand, public state, progression, and available options predict that choice?
3. How stable is the recommendation across stronger policies, increased search budgets, and evaluation objectives?

Initially use the official **two-player** configuration. Never equate high score with victory; retain both.

## Two timelines: official and experimental

**Official trajectory:** execute rules until the first official terminal trigger. Freeze official score and win/loss/draw there, including the exact player(s), cause, and global event clock.

**Shadow trajectory:** fork when the first official terminal trigger occurs and advance **both colonies** to a common horizon for a comparable developmental score. All first-death markers persist; never reset them. A colony that dies later during shadow continuation also gets a first-death marker and switches to its configured fixed post-death policy.

Shadow play is explicitly **not legal official play**. Document what happens when official rules require ending the game or have undefined post-death transitions. Use an explicit shadow adapter, not hidden normal-rule changes. Record whether death is measured by population reaching zero, inability to pay a loss, or another verified rule condition. Preserve the signed population/debt state in shadow mode if it is coherent; do not silently clamp or fabricate people. If negative population breaks rule semantics, record a virtual deficit separately from legal population and define the adapter via tested, versioned conventions.

**Post-death policy:** deterministic fixed action mapping from the allowed shadow observation to an action, configured independently from the normal agent. Do not temporarily optimize population, switch back to normal play at positive population, or use resurrection as evidence that the death was acceptable. If there are no legal actions, apply a documented no-action advance. Its precise priority order is a configurable research convention to decide after legal actions are known.

**Horizon:** use a single common end condition, preferably the natural end of the verified progression system *if unambiguous*. If a full deck/round horizon is undefined, explicitly specify a bounded experimental horizon with a versioned setting. Never compare shadow scores at different timestamps as though equivalent.

**Timing:** record (round or deck cycle, event position within cycle, absolute number of revealed progression events, decision count, terminal cause). A player surviving to the horizon has `death_time=null`; this is not the same as death at the final tick.

## Evaluation objective: preserve components, not a single magic reward

Per player record:
- `official_outcome` and `official_terminal_score` (both players)
- `shadow_final_score` (both players at common horizon)
- `first_death_time` and `first_death_cause` (optional per player)
- optional `death_deficit`, shadow recovery information, and rules/adapter versions

Primary **research hypothesis**: own shadow score conveys more learning signal than win/loss alone, even when the opponent was strong or died early. However, an early death must not be compensated freely by building wealth then recovering artificially.

Therefore start with a **multi-output value model / vector-valued rollout**, not a prematurely fixed scalar: expected own and opponent shadow scores, survival probability, death-time distribution, and official win chance. Explore different scalarizations offline and measure their impact on official win rate.

Two candidate ranking policies to test:

1. **Death-first lexicographic**: official survival / first-death timing dominates shadow score; among comparable mortality outcomes, maximize `S_self_shadow - λ*S_opp_shadow`. This is conservative and may be too risk-averse; it is an explicit hypothesis.
2. **Smooth risk penalty**: `U = S_self_shadow - λ*S_opp_shadow - α * f(first_death_time, severity, horizon)`, with nonnegative configurable `α`, and `f` strictly penalizing earlier death; dead-after-positive-recovery remains penalized. For comparative modeling, also test an official-outcome component. Calibrate units and normalize features from real rollouts rather than arbitrary constants.

Important: a raw death timestamp compared between games of different lengths is invalid. Compare to the same common event clock/horizon, and condition on actual game history. A death penalty should be **monotone earlier-is-worse** and should not reward negative population or repeated simulated death/recovery cycling. For any finite penalty there may be shadow scores high enough to outweigh death; test adversarial cases and use a lexicographic or capped-score alternative if that exploit appears.

Do not train an agent to optimize shadow score alone without evaluating against official wins. Distinguish **learning signal** from **actual strategic objective**.

## Shared-deck externalities: paired counterfactuals

When adding event / robot / development cards could help or hurt the opponent:

- Fork the same observable decision state into `add` and `do_not_add` where actions are legal and the alternative is meaningful.
- Let the opponent choose using its *own information* in each branch; never expose the counterfactual branch or hidden card order.
- Use shared exogenous random streams when a mathematically sound coupling is available; added deck items alter draw distributions and cannot be paired by blindly matching next-card indices.
- Measure changes to **both players' shadow score, death distributions, and official outcomes** with confidence intervals. Include altered game length and interactions.
- Avoid crediting opponent behavior due solely to different unrelated initial states. Report sample counts, seeds, and coupling scheme.
- Store both counterfactual outcomes; use deltas to refine predicted `S_opp` without assigning a fixed static cost to a card.

## First-Work research protocol

Sample many legal initial states from the true setup distribution. At the **first actual Work decision** capture the player's allowed observation. For every legal first action (including all meaningful BUILD targets), evaluate with equal compute and identical continuation-agent family. Compare Research/draw against the highest-valued non-Research action for each observation. Use paired rollouts and confidence intervals, remembering that selecting the apparent best from noisy estimates creates selection bias; reserve independent evaluation samples for final comparisons. Report average advantage, frequency Research is best, and conditional effects by hand and public progression features.

Use **official win rate as the final external validation**, even when selecting or training primarily with scores and mortality. Track search-budget performance and agreement of multiple strong agent variants before claiming a conclusion.

## Baselines and learning

- Deterministic/legal random baseline and simple heuristics are *baselines*, not trusted strategic judges.
- Prototype PIMC for comparison, and information-set MCTS for the main search candidate. Handle simultaneous decisions and hidden information explicitly; benchmark alternatives instead of assuming ISMCTS is optimal.
- Later train separate value heads for official outcome, own/opponent shadow score, death timing and optional score distributions. Compare to no-learning search at **fixed total compute**, not merely fixed simulation count.
- Self-play against historical snapshots to identify genuine improvements. Opponent matchups are a validation tool, not the central scientific hypothesis.

## Statistical hygiene

Capture uncertainty (bootstrap/paired confidence intervals), multiple seeds, independent held-out evaluation scenarios, and exact versions. Analyze by both *games simulated* and *CPU/GPU time*. Never infer optimal play from a weak baseline or one stable self-play run.
