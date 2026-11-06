#include "engine/progression.hpp"

#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace bloodbath::progression {
namespace {
void increment(std::uint64_t& value) {
    if (value == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("progression counter overflow");
    ++value;
}
void shuffle(std::vector<Card>& cards, const RandomStreams& rng,
             std::uint64_t event, bool additions) {
    for (std::size_t size = cards.size(); size > 1; --size) {
        // Distinct parity addresses separate shuffle and addition randomness.
        const auto draw = static_cast<std::uint64_t>(size) * 2 + (additions ? 1 : 0);
        const auto index = rng.bounded(RandomSource::progression, event, draw, size);
        std::swap(cards[size - 1], cards[static_cast<std::size_t>(index)]);
    }
}
std::set<Card> used_cards(const Snapshot& state) {
    std::set<Card> used;
    const auto insert = [&](Card card) {
        if (!used.insert(card).second) throw std::invalid_argument("duplicate progression instance");
    };
    for (const auto card : state.deck) insert(card);
    for (const auto card : state.discard) insert(card);
    if (state.active) insert(*state.active);
    return used;
}
} // namespace
Deck::Deck(Snapshot state, std::uint64_t seed) : state_(std::move(state)), seed_(seed) {
    (void)used_cards(state_);
}
Observation Deck::observe() const {
    auto visible = state_.active;
    if (!visible && !state_.discard.empty()) visible = state_.discard.back();
    return {visible, state_.clock};
}
std::optional<Reveal> Deck::reveal() {
    if (state_.active) throw std::invalid_argument("active card still has unresolved work");
    if (state_.deck.empty() && state_.discard.empty()) return std::nullopt;
    auto next = state_;
    const bool reshuffled = next.deck.empty();
    if (reshuffled) {
        increment(next.shuffle_index);
        increment(next.clock.cycle);
        next.clock.position = 0;
        next.deck = std::move(next.discard);
        next.discard.clear();
        shuffle(next.deck, RandomStreams(seed_), next.shuffle_index, false);
    }
    increment(next.clock.absolute_event);
    increment(next.clock.position);
    next.active = next.deck.back();
    next.deck.pop_back();
    const Reveal result{*next.active, next.clock, reshuffled};
    state_ = std::move(next);
    return result;
}
void Deck::finish_active() {
    if (!state_.active) throw std::invalid_argument("no active progression card");
    state_.discard.push_back(*state_.active);
    state_.active.reset();
}
void Deck::add_group(std::vector<Card> cards) {
    if (cards.empty()) return;
    auto used = used_cards(state_);
    for (const auto card : cards)
        if (!used.insert(card).second) throw std::invalid_argument("duplicate progression addition");
    auto next = state_;
    if (cards.size() > 1) {
        increment(next.addition_index);
        shuffle(cards, RandomStreams(seed_), next.addition_index, true);
    }
    next.deck.insert(next.deck.end(), cards.begin(), cards.end());
    state_ = std::move(next);
}
void Deck::record_decision() {
    if (!state_.active) throw std::invalid_argument("no active card for decision");
    increment(state_.clock.decision);
}
} // namespace bloodbath::progression
