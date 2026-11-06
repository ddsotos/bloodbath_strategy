#include "engine/population.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace bloodbath::population {
namespace {
void validate(const Colony& colony) {
    std::set<Card> used;
    for (const auto& building : colony.buildings)
        if (!used.insert(building.card).second) throw std::invalid_argument("duplicate building");
    for (const auto card : colony.discarded_buildings)
        if (!used.insert(card).second) throw std::invalid_argument("duplicate discarded building");
    (void)total_people(colony);
}
} // namespace
std::uint64_t total_people(const Colony& colony) {
    auto total = colony.base_people;
    for (const auto& building : colony.buildings) {
        if (building.printed_people > std::numeric_limits<std::uint64_t>::max() - total)
            throw std::overflow_error("population overflow");
        total += building.printed_people;
    }
    return total;
}
Loss::Loss(Colony colony, Player player, std::uint64_t amount, EventClock clock, std::string cause)
    : colony_(std::move(colony)), player_(player), clock_(clock), cause_(std::move(cause)),
      result_{Status::needs_building_choice, amount, 0, 0, std::nullopt, {}} {
    if (player > 1 || cause_.empty()) throw std::invalid_argument("invalid loss context");
    validate(colony_);
    if (total_people(colony_) == 0)
        throw std::invalid_argument("loss requires a living initial colony; prior death time is unknown");
    // DeathRecord currently stores a signed deficit; fail instead of truncating it.
    if (amount > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        throw std::overflow_error("loss exceeds death-record range");
    advance();
}
void Loss::advance() {
    if (total_people(colony_) < result_.requested_loss && !colony_.buildings.empty()) {
        // End interruption vs Building/LOST resolution order is not yet verified.
        result_.unpaid_shortfall = result_.requested_loss - total_people(colony_);
        result_.status = Status::needs_terminal_rule;
    } else if (colony_.base_people >= result_.requested_loss) {
        colony_.base_people -= result_.requested_loss;
        result_.paid = result_.requested_loss;
        result_.status = total_people(colony_) == 0 ? Status::failed : Status::paid;
    } else if (colony_.buildings.empty()) {
        // Preserve the pre-payment official state: an impossible payment is not negative people.
        result_.unpaid_shortfall = result_.requested_loss - colony_.base_people;
        result_.status = Status::failed;
    } else {
        result_.status = Status::needs_building_choice;
    }
    if (result_.status == Status::failed && !result_.death)
        result_.death = DeathRecord{player_, clock_, cause_,
                                   static_cast<std::int64_t>(result_.unpaid_shortfall)};
}
std::vector<Card> Loss::choices() const {
    std::vector<Card> cards;
    if (result_.status == Status::needs_building_choice)
        for (const auto& building : colony_.buildings) cards.push_back(building.card);
    return cards;
}
void Loss::choose_building(Card card) {
    if (result_.status != Status::needs_building_choice)
        throw std::invalid_argument("loss is not awaiting a building choice");
    const auto it = std::find_if(colony_.buildings.begin(), colony_.buildings.end(),
                                 [&](const Building& building) { return building.card == card; });
    if (it == colony_.buildings.end()) throw std::invalid_argument("building not owned");
    if (it->lost_effects != LostEffects::caller_verified_none)
        throw std::invalid_argument("LOST effects must be implemented before choosing this building");
    auto next = colony_;
    auto next_result = result_;
    next.base_people += it->printed_people; // total_people validation guarantees no overflow.
    next.discarded_buildings.push_back(card);
    next_result.lost.push_back({card, it->printed_people, it->boxes});
    next.buildings.erase(next.buildings.begin() + (it - colony_.buildings.begin()));
    colony_ = std::move(next);
    result_ = std::move(next_result);
    advance();
}
} // namespace bloodbath::population
