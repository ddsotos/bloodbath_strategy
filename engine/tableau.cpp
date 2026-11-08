#include "engine/tableau.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>

namespace bloodbath::tableau {
namespace {
void add(std::uint64_t& value, std::uint64_t amount) {
    if (amount > std::numeric_limits<std::uint64_t>::max() - value)
        throw std::overflow_error("tableau resource overflow");
    value += amount;
}
void player(Player p) { if (p > 1) throw std::invalid_argument("invalid player"); }
Kind known(const Catalog& catalog, Card card) {
    const auto found = catalog.find(card);
    if (found == catalog.end() || (found->second != Kind::kale_farm && found->second != Kind::flag))
        throw std::invalid_argument("unsupported Building definition");
    return found->second;
}
void farm(base_rules::State& state, const Catalog& catalog, Player p, std::vector<EffectRecord>& effects) {
    auto& colony = state.colonies[p];
    add(colony.resources.food, 4);
    effects.push_back({p, EffectKind::farm_base, std::nullopt, 4});
    for (const auto card : colony.buildings) {
        if (known(catalog, card) == Kind::kale_farm) {
            add(colony.resources.food, 1);
            effects.push_back({p, EffectKind::farm_bonus, card, 1});
        }
    }
}
} // namespace
std::uint64_t total_people(const base_rules::Colony& colony, const Catalog& catalog) {
    auto total = colony.resources.people;
    for (const auto card : colony.buildings) add(total, printed_people(known(catalog, card)));
    return total;
}
std::uint64_t hunger_food_cost(const base_rules::Colony& colony, const Catalog& catalog) {
    std::uint64_t total = 0;
    for (const auto card : colony.buildings) {
        if (known(catalog, card) != Kind::flag) add(total, 1);
    }
    return total;
}
std::uint64_t build_cost(Kind kind) {
    if (kind == Kind::kale_farm) return 2;
    if (kind == Kind::flag) return 0;
    throw std::invalid_argument("unsupported Building definition");
}
std::uint64_t printed_people(Kind kind) {
    if (kind == Kind::kale_farm) return 5;
    if (kind == Kind::flag) return 0;
    throw std::invalid_argument("unsupported Building definition");
}
void validate(const base_rules::State& state, const Catalog& catalog, base_rules::EffectScope scope) {
    if (scope != base_rules::EffectScope::caller_verified_unmodified)
        throw std::invalid_argument("unverified external tableau modifiers");
    auto check = state;
    (void)base_rules::draw_buildings(check, {0, 0}, {0, 1}, 0,
                                    base_rules::DrawScope::caller_verified_unmodified_draw);
    std::set<Card> physical;
    for (const auto& colony : state.colonies) {
        physical.insert(colony.hand.begin(), colony.hand.end());
        physical.insert(colony.buildings.begin(), colony.buildings.end());
        (void)total_people(colony, catalog);
    }
    physical.insert(state.building_deck.begin(), state.building_deck.end());
    physical.insert(state.building_discard.begin(), state.building_discard.end());
    for (const auto& [card, kind] : catalog)
        if ((kind != Kind::kale_farm && kind != Kind::flag) || !physical.contains(card))
            throw std::invalid_argument("invalid subset Building catalog entry");
}
void validate_choice(const base_rules::State& state, const Catalog& catalog, Player p, const Choice& choice) {
    player(p);
    const auto& colony = state.colonies[p];
    if (choice.action == base_rules::WorkAction::build) {
        if (!choice.build || !choice.box_targets.empty()) throw std::invalid_argument("BUILD needs one target");
        known(catalog, *choice.build);
        if (std::find(colony.hand.begin(), colony.hand.end(), *choice.build) == colony.hand.end() ||
            colony.resources.money < build_cost(known(catalog, *choice.build)))
            throw std::invalid_argument("BUILD target is not affordable/owned");
    } else if (choice.action == base_rules::WorkAction::restock) {
        const auto quota = colony.buildings.empty() ? std::size_t{0} : std::size_t{2};
        if (choice.build || choice.box_targets.size() != quota)
            throw std::invalid_argument("RESTOCK requires available two-box allocation");
        for (const auto card : choice.box_targets)
            if (std::find(colony.buildings.begin(), colony.buildings.end(), card) == colony.buildings.end())
                throw std::invalid_argument("box target must be an owned Building");
    } else if (choice.action != base_rules::WorkAction::mine && choice.action != base_rules::WorkAction::farm &&
               choice.action != base_rules::WorkAction::research) {
        throw std::invalid_argument("invalid tableau Work action");
    } else if (choice.build || !choice.box_targets.empty()) {
        throw std::invalid_argument("unexpected Work targets");
    }
}
std::vector<Choice> options(const base_rules::State& state, const Catalog& catalog, Player p) {
    player(p);
    std::vector<Choice> result{{base_rules::WorkAction::mine}, {base_rules::WorkAction::farm},
                               {base_rules::WorkAction::research}};
    const auto& colony = state.colonies[p];
    for (const auto card : colony.hand)
        if (catalog.contains(card) && colony.resources.money >= build_cost(catalog.at(card)))
            result.push_back({base_rules::WorkAction::build, card});
    if (colony.buildings.empty()) result.push_back({base_rules::WorkAction::restock});
    else
        for (std::size_t i = 0; i < colony.buildings.size(); ++i)
            for (std::size_t j = i; j < colony.buildings.size(); ++j)
                result.push_back({base_rules::WorkAction::restock, std::nullopt,
                                  {colony.buildings[i], colony.buildings[j]}});
    return result;
}
population::Result lose_people(base_rules::State& state, const Catalog& catalog, Player p,
                               std::uint64_t amount, const std::vector<Card>& chosen_buildings,
                               EventClock clock, std::string cause, base_rules::EffectScope scope) {
    player(p);
    validate(state, catalog, scope);
    const auto& colony = state.colonies[p];
    if (total_people(colony, catalog) <= amount)
        throw std::invalid_argument("terminal Building loss ordering is unsupported");
    population::Colony input{colony.resources.people, {}, {}};
    for (const auto card : colony.buildings) {
        const auto boxes = colony.boxes.find(card);
        const auto printed = printed_people(known(catalog, card));
        const auto box_count = boxes == colony.boxes.end() ? 0 : boxes->second;
        input.buildings.push_back({card, printed, box_count, population::LostEffects::caller_verified_none});
    }
    population::Loss loss(std::move(input), p, amount, clock, std::move(cause));
    for (const auto card : chosen_buildings) loss.choose_building(card);
    if (loss.status() != population::Status::paid)
        throw std::invalid_argument("incomplete Building loss choices");
    auto next = state;
    auto& target = next.colonies[p];
    target.resources.people = loss.snapshot().base_people;
    for (const auto& lost : loss.result().lost) {
        target.buildings.erase(std::find(target.buildings.begin(), target.buildings.end(), lost.card));
        target.boxes.erase(lost.card);
        next.building_discard.push_back(lost.card);
    }
    validate(next, catalog, scope);
    auto result = loss.result();
    state = std::move(next);
    return result;
}
WorkRecord resolve(base_rules::State& state, const Catalog& catalog, const std::array<Choice, 2>& choices,
                   std::array<Player, 2> schedule, std::uint64_t seed, base_rules::EffectScope scope) {
    validate(state, catalog, scope);
    if (!((schedule[0] == 0 && schedule[1] == 1) || (schedule[0] == 1 && schedule[1] == 0)))
        throw std::invalid_argument("invalid tableau schedule");
    for (Player p = 0; p < 2; ++p) validate_choice(state, catalog, p, choices[p]);
    auto next = state;
    WorkRecord record{{{choices[0].action, choices[1].action}, schedule, {}, seed,
                       state.building_shuffle_index, state.building_shuffle_index}, choices, {}};
    for (const Player p : schedule) {
        auto& colony = next.colonies[p];
        const auto& choice = choices[p];
        switch (choice.action) {
        case base_rules::WorkAction::build: {
            const auto card = *choice.build;
            const auto kind = known(catalog, card);
            const auto cost = build_cost(kind);
            colony.resources.money -= cost;
            record.effects.push_back({p, EffectKind::pay_money, card, cost});
            colony.hand.erase(std::find(colony.hand.begin(), colony.hand.end(), card));
            colony.buildings.push_back(card);
            colony.boxes.emplace(card, 0);
            record.effects.push_back({p, EffectKind::enter_play, card, printed_people(kind)});
            if (kind == Kind::kale_farm) farm(next, catalog, p, record.effects);
            else {
                add(colony.resources.money, 6);
                record.effects.push_back({p, EffectKind::money_gain, card, 6});
            }
            break;
        }
        case base_rules::WorkAction::restock:
            for (const auto card : choice.box_targets) {
                add(colony.boxes[card], 1);
                record.effects.push_back({p, EffectKind::place_box, card, 1});
            }
            break;
        case base_rules::WorkAction::farm: farm(next, catalog, p, record.effects); break;
        case base_rules::WorkAction::mine:
            add(colony.resources.money, 4);
            record.effects.push_back({p, EffectKind::mine_base, std::nullopt, 4});
            break;
        case base_rules::WorkAction::research: {
            std::array<std::uint32_t, 2> counts{};
            counts[p] = 2;
            const auto draws = base_rules::draw_buildings(next, counts, schedule, seed,
                base_rules::DrawScope::caller_verified_unmodified_draw);
            record.base.drawn[p] = draws.drawn[p];
            for (const auto card : draws.drawn[p]) record.effects.push_back({p, EffectKind::draw_card, card, 1});
            break;
        }
        }
    }
    record.base.shuffles_after = next.building_shuffle_index;
    for (const auto& colony : next.colonies) (void)total_people(colony, catalog);
    state = std::move(next);
    return record;
}
} // namespace bloodbath::tableau
