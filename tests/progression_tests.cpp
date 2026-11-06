#include "engine/progression.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace progress = bloodbath::progression;
using bloodbath::EventClock;

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class Exception = std::invalid_argument, class F> void rejects(F function) {
    try { function(); } catch (const Exception&) { return; }
    throw std::runtime_error("expected rejection");
}
// IDs below are synthetic physical tokens, not an official Progress catalog.
progress::Snapshot fixture(std::vector<progress::Card> deck,
                           std::vector<progress::Card> discard = {}) {
    progress::Snapshot state;
    state.deck = std::move(deck);
    state.discard = std::move(discard);
    return state;
}
std::vector<progress::Card> all_cards(const progress::Snapshot& state) {
    auto cards = state.deck;
    cards.insert(cards.end(), state.discard.begin(), state.discard.end());
    if (state.active) cards.push_back(*state.active);
    std::sort(cards.begin(), cards.end());
    require(std::set<progress::Card>(cards.begin(), cards.end()).size() == cards.size(),
            "duplicate physical token in snapshot");
    return cards;
}
void deferred_shuffle_and_insertion() {
    const auto initial = fixture({1}, {2});
    progress::Deck deck(initial, 10);
    const auto first = deck.reveal().value();
    require(first.card == 1 && !first.reshuffled && first.clock == EventClock{1, 0, 1, 0},
            "first reveal clock or card incorrect");
    require(deck.snapshot().deck.empty() && deck.snapshot().discard == std::vector<progress::Card>{2},
            "emptying deck reshuffled prematurely");
    const auto pending = deck.snapshot();
    rejects([&] { (void)deck.reveal(); });
    require(deck.snapshot() == pending, "blocked reveal changed state");
    deck.record_decision();
    deck.record_decision();
    require(deck.snapshot().clock == EventClock{1, 0, 1, 2}, "decision advanced progression clock");
    deck.add_group({3});
    require(deck.snapshot().active == progress::Card{1} && deck.snapshot().shuffle_index == 0,
            "addition resolved active card or shuffled discards");
    rejects([&] { (void)deck.reveal(); });
    deck.finish_active();
    require(deck.observe().current_or_top_discard == progress::Card{1}, "finished card not visible on discard");
    const auto next = deck.reveal().value();
    require(next.card == 3 && !next.reshuffled && next.clock == EventClock{2, 0, 2, 2},
            "single addition not next or changed cycle");
    deck.finish_active();
    const auto before_recycle = all_cards(deck.snapshot());
    const auto recycled = deck.reveal().value();
    require(recycled.reshuffled && recycled.clock == EventClock{3, 1, 1, 2},
            "needed recycle did not start next cycle");
    require(deck.snapshot().shuffle_index == 1 && deck.snapshot().discard.empty(),
            "recycle count or discard incorrect");
    require(all_cards(deck.snapshot()) == before_recycle, "recycle lost physical cards");

    progress::Deck no_addition(initial, 10);
    (void)no_addition.reveal();
    no_addition.finish_active();
    require(no_addition.snapshot().shuffle_index == 0, "finish prematurely reshuffled");
    const auto second = no_addition.reveal().value();
    require(second.reshuffled && second.clock.absolute_event == 2 && second.clock.position == 1,
            "no-addition next reveal failed to recycle");
}
void observation_privacy_and_invalid_operations() {
    auto a = fixture({10, 11}, {20, 21});
    auto b = fixture({31, 30}, {22, 21});
    progress::Deck left(a, 1), right(b, 2);
    require(left.observe() == right.observe(), "hidden future or buried discard leaked");
    require(left.observe().current_or_top_discard == progress::Card{21}, "visible discard omitted");
    a.active = 40;
    b.active = 40;
    progress::Deck active_left(a, 1), active_right(b, 2);
    require(active_left.observe() == active_right.observe() &&
            active_left.observe().current_or_top_discard == progress::Card{40}, "active visibility incorrect");
    rejects([] { (void)progress::Deck(fixture({1, 1}), 0); });
    rejects([] { (void)progress::Deck(fixture({1}, {1}), 0); });
    auto duplicate_active = fixture({1});
    duplicate_active.active = 1;
    rejects([&] { (void)progress::Deck(duplicate_active, 0); });
    const auto before = active_left.snapshot();
    rejects([&] { active_left.add_group({40}); });
    rejects([&] { active_left.add_group({50, 50}); });
    rejects([&] { active_left.add_group({50, 10}); });
    require(active_left.snapshot() == before, "invalid additions partially applied");
    active_left.add_group({});
    require(active_left.snapshot() == before, "empty addition changed state");
    progress::Deck empty(fixture({}), 0);
    const auto empty_before = empty.snapshot();
    require(!empty.reveal(), "empty deck fabricated a card");
    rejects([&] { empty.finish_active(); });
    rejects([&] { empty.record_decision(); });
    require(empty.snapshot() == empty_before, "empty operation changed state");
}
void grouped_additions_and_restore() {
    std::set<std::vector<progress::Card>> permutations;
    for (std::uint64_t seed = 0; seed < 32; ++seed) {
        const auto initial = fixture({1, 2}, {3, 4});
        progress::Deck first(initial, seed), same(initial, seed);
        first.add_group({7, 8, 9});
        same.add_group({7, 8, 9});
        const auto added = first.snapshot();
        require(added == same.snapshot(), "group addition not seed reproducible");
        require(added.deck.size() == 5 && added.deck[0] == 1 && added.deck[1] == 2 &&
                added.discard == initial.discard && added.addition_index == 1 &&
                added.shuffle_index == 0, "group addition changed unrelated zones");
        std::vector<progress::Card> suffix(added.deck.begin() + 2, added.deck.end());
        permutations.insert(suffix);
        std::sort(suffix.begin(), suffix.end());
        require(suffix == std::vector<progress::Card>{7, 8, 9}, "group additions missing or mixed");
        const auto expected = all_cards(added);
        progress::Deck restored(added, seed);
        for (int event = 0; event < 18; ++event) {
            const auto one = first.reveal().value();
            const auto two = restored.reveal().value();
            require(one == two && first.snapshot() == restored.snapshot(), "restored reveal diverged");
            if (event < 3) require(one.card >= 7 && one.card <= 9, "added group not on top");
            require(all_cards(first.snapshot()) == expected, "reveal lost card instances");
            if (event % 2 == 0) { first.record_decision(); restored.record_decision(); }
            first.finish_active();
            restored.finish_active();
            require(first.snapshot() == restored.snapshot(), "restored finish or clock diverged");
        }
    }
    require(permutations.size() > 1, "addition shuffle ignored seed or left group unchanged");
}
void overflow_atomicity() {
    constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
    for (int counter = 0; counter < 4; ++counter) {
        auto state = counter < 2 ? fixture({1}) : fixture({}, {1});
        if (counter == 0) state.clock.absolute_event = maximum;
        if (counter == 1) state.clock.position = maximum;
        if (counter == 2) state.clock.cycle = maximum;
        if (counter == 3) state.shuffle_index = maximum;
        progress::Deck deck(state, 0);
        rejects<std::overflow_error>([&] { (void)deck.reveal(); });
        require(deck.snapshot() == state, "reveal overflow partially mutated snapshot");
    }
    auto addition_state = fixture({1}, {2});
    addition_state.addition_index = maximum;
    progress::Deck addition(addition_state, 0);
    rejects<std::overflow_error>([&] { addition.add_group({3, 4}); });
    require(addition.snapshot() == addition_state, "addition overflow partially changed state");
    addition.add_group({3});
    require(addition.snapshot().addition_index == maximum && addition.snapshot().deck.back() == 3,
            "single addition unnecessarily consumed shuffle identity");
    auto decision_state = fixture({});
    decision_state.active = 1;
    decision_state.clock.decision = maximum;
    progress::Deck decision(decision_state, 0);
    rejects<std::overflow_error>([&] { decision.record_decision(); });
    require(decision.snapshot() == decision_state, "decision overflow changed state");
}
} // namespace

int main() {
    try {
        deferred_shuffle_and_insertion();
        observation_privacy_and_invalid_operations();
        grouped_additions_and_restore();
        overflow_atomicity();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "progression contracts passed with synthetic physical-card fixtures\n";
}
