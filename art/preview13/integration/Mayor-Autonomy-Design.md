# City Evolution — autonomous mayor design

5 November 2026. Proposed behavior for the current code rewrite.

This answers the design question; it is not a claim that the current Mayor
button already runs this controller. No engine code is changed by this document.

## Player controls and freedom

| Control | Behavior |
| --- | --- |
| Mayor: Off | Player controls construction. The controller makes no changes. |
| Mayor: Advice | Diagnose problems, display proposed projects, expected cost and reason; do not execute. |
| Mayor: Autonomous | Choose, place and connect projects automatically without per-project approval dialogs. |
| Area: selected district | Every tile of each new building, road, bridge, utility or demolition must be inside the selected district's saved mask. Existing infrastructure outside the mask can still be used by the simulation. |
| Area: whole map | Same planner, with all buildable map tiles available. |
| Spending settings | Construction allowance, reserve, recurring-upkeep allowance and borrowing permission. Full authority can spend freely within the player's chosen financial policy and the game's legal treasury rules. |
| Redevelopment setting | Decide whether the mayor may remove existing player buildings. Off by default; separate from ordinary clear-land construction. |

Autonomous means real construction authority over **all unlocked building
types**, rather than a fixed list of a few scripted buildings. It does not
unlock future technology, create free materials, ignore terrain or spend
nonexistent money. Give the mayor an explicit Stop/Pause control and a visible
project log. Turning autonomy off cancels uncommitted actions immediately.

The area selector determines authority, not a vague preference. A road or
multi-tile building partly outside the chosen district is not allowed. If the
district lacks room for a solution, explain the blocked project rather than
silently expanding the mayor's jurisdiction.

## What he should actually reason about

Use a local, deterministic game controller: needs assessment, scored projects,
pathfinding and an execution queue. No online chat model, API key or network
request is needed for this behavior. Natural-language explanations can be
generated from the same reason codes that selected each project.

Work from the simulation's actual data, not decorative moving-car counts:

- Housing vacancies, unhoused population, desirability and active service coverage.
- Employment demand and available labor, idle production capacity and supply chains.
- Commodity inventories, market reach, transport connectivity and delivery bottlenecks.
- Power generation, network connectivity and spare capacity; water and waste services.
- Treasury trend, committed construction, operating surplus and recurring upkeep.
- Technology prerequisites, terrain, pollution and available land.
- Route utilization, freight movement and passenger demand where those metrics exist.

If congestion is currently only a visual effect, expose route/capacity metrics
in the rewrite before claiming that the mayor can solve measured congestion.
An initial transport heuristic can use connectivity, trip demand and known
bottlenecks, but the UI should describe it as an estimate.

## Diagnose before choosing a building

| Situation | First checks | Candidate projects |
| --- | --- | --- |
| Economy losing money | Operating costs versus revenue, idle capacity, supply shortages, export access | Reconnect or supply existing production; build a productive chain only when labor, demand, inputs and upkeep support it. A factory is not automatically profitable. |
| Unemployment | Job shortage versus unstaffed workplaces that cannot obtain inputs; skill requirements | Repair supply/access, then add an appropriate unlocked employer and its necessary connections. |
| Congested corridor | Disconnected routes, a single choke point, excessive transfers, freight/passenger overlap | Complete a missing link, improve a junction, add a justified bypass or transit capacity. Avoid parallel dead-end roads. |
| Housing shortage | Empty usable homes, service failures and commute access | Improve existing coverage or build suitable homes with road and utility access. Use towers only when late-game density demand and services justify them. |
| Power shortage | Disconnected substations or lines versus insufficient generation | Repair network connectivity first; expand generation only when needed. |
| Low education/health/happiness | Existing service operation and supply, coverage gaps, recurring affordability | Supply or connect a service; otherwise place a school, clinic, church, park or other appropriate amenity near the uncovered homes. |
| Food/goods shortage | Stock, production, labor, distribution and market capacity | Repair the weakest chain segment, rather than building an unrelated expensive producer. |

These are diagnostic patterns, not unconditional if-statements such as
"low money → factory" or "cars visible → road."

## Project selection and execution

1. **Observe trends.** Use recent simulation history as well as today's values.
   A short-lived shortage should not trigger a permanent expensive project.
2. **Generate candidates.** Consider repairs/connections, upgrades, appropriate
   services, production chains and expansion. All unlocked construction types
   can participate through a shared construction catalog.
3. **Plan the whole project.** A factory may need an access road, power,
   supplies and market/export reach. Include dependency paths, footprints,
   full construction cost and projected upkeep in one proposal.
4. **Reject invalid proposals.** Check scope, terrain, technology, collision,
   protected sites, affordability, labor and credible input/output access.
5. **Score useful proposals.** Combine need severity, people/jobs served,
   expected bottleneck relief and financial benefit; subtract construction,
   upkeep, pollution, disruption and uncertain benefits. Emergency service
   restoration takes precedence over optional growth.
6. **Reserve resources for one selected project.** Include queued costs, so
   two individually affordable proposals cannot jointly overspend. Check
   construction cash and a conservative upkeep forecast separately.
7. **Execute through the normal construction commands.** Revalidate each
   action against the current world, current authority mask and treasury.
   Charge normal costs and preserve normal simulation side effects. Do not
   directly write building IDs into map tiles.
8. **Observe the result.** Record baseline, action, expected benefit and actual
   outcome. Allow the supply chain time to settle before adding the same kind
   of project again. Cancel remaining dependencies if their goal becomes
   invalid; do not bulldoze a functioning completed project automatically.

A compact controller loop:

```text
if mode is Off: cancel uncommitted plan and return
update bounded batches of city/district observations
if observations are ready and no active project:
    diagnose needs -> generate projects -> validate -> score
    choose a positive-benefit affordable project, or explain why none qualifies
if mode is Advice: publish proposal and return
if an execution step is due:
    revalidate the next action and the whole remaining budget
    execute one normal game command, or cancel/replan with a reason
periodically compare completed project outcomes against their baseline
```

## Stability, performance and player trust

- Use different start/stop thresholds and project cooldowns so the mayor does
  not repeatedly build and remove the same service or road.
- Penalize repeated failed proposals; remember why a location failed. Retry
  when the relevant condition changes, not every frame.
- Keep protected/player-designated sites and never demolish outside the
  explicit redevelopment permission. Full-map authority does not change that
  setting unless the player chooses it.
- Keep construction and recurring expenses visible. If no affordable useful
  project exists, waiting with a reason is a valid decision.
- Spread observation and path searches across bounded work batches. Do not
  scan the whole map or run many route searches every rendered frame.
- Schedule decisions from simulation time; use stable tie-breakers and saved
  planner state so save/reload does not randomize pending construction.
- Revalidate affected tiles after player actions. Never execute a stale path
  across newly placed buildings or a changed district boundary.
- In the log, write concrete reasons: “Connected the existing mill to its
  market; it was idle because goods could not reach it.” Include cost, area
  and outcome rather than a generic “AI improved the economy” message.

## Implementation boundary for the rewrite

Integrate through separate components for observations, candidate planning,
normal construction commands, persistent policy/state and the Mayor UI. Use
the rewrite's current types and save format. Do not copy an older game tree
over that work. Share placement/path validation between player tools and the
mayor, so permission and cost rules cannot diverge.

The controller should initially prefer projects whose effects are measurable
in the existing simulation. Adding new economic behavior, such as revenue
from a previously decorative office tower, requires separate simulation work.
Skyscraper pictures alone do not create jobs, housing capacity or taxes.

## Evidence required before calling it functional

Run automated integration scenarios that verify real map and treasury changes:

- Advice mode changes nothing; autonomy Off stops queued construction.
- A district-edge 3×3 building, crossing road and bridge are rejected whenever
  any newly changed tile lies outside the authority mask.
- An unemployed, supplied settlement gets a useful connected employer, while
  an input-starved industrial district first gets its broken supply repaired.
- A genuinely disconnected corridor gets a useful connection; dense decorative
  cars alone do not trigger road spam.
- No overspending across project dependencies; upkeep/reserve policy respected.
- Technology locks, existing protected buildings and demolition permission work.
- A player building on the pending path forces a fresh valid plan.
- Save/reload preserves policy, reservations, pending goals and cooldowns.
- Prolonged simulation does not produce repeated construction loops or bankrupt
  a stable baseline settlement through unnecessary maintenance.
- The GUI demonstrates the mayor constructing and connecting a complete
  useful project without player placement clicks.

This is achievable game logic. It needs measured testing and tuning; no local
planner can promise that every possible city or crisis is always solvable.
