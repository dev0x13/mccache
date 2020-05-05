#pragma once

#include <vector>
#include <cstddef>
#include <cassert>

#include "vector.h"

// Stats accumulator interface. Used for collecting transitions statistics,
// collected with different transitions interpretations.
class StatsAccumulator {
public:
    // Registers new state
    virtual void addState() = 0;

    // Collects transition from state1 to state2
    virtual void accumulateTransition(size_t state1, size_t state2) = 0;

    // Returns unnormalized (i.e. the sum of elements != 1) posterior transitions probabilities
    // from given state. Output transitions vector should be preallocated to have <number of states> size.
    virtual void getTransitionProbabilitiesEstimate(size_t state, Vector<float>* transitions) = 0;

    // Returns a single unnormalized posterior transition probability from state1 to state2
    virtual float getTransitionProbabilityEstimate(size_t state1, size_t state2) const = 0;

    virtual ~StatsAccumulator() = default;
};

// Stats accumulator implementation, which employs transitions stats taking into account
// only the "length" of transitions (i.e. |state1 - state2|).
class TransitionsBasedStatsAccumulator : public StatsAccumulator {
public:
    // Contains total numbers of forward (state1 < state2) transitions for
    // each length. Index of vector == length of the transition. Thus zeroth element
    // contains nothing and should not be used.
    std::vector<float> totalNumbersOfForwardTransitions;

    // Contains total numbers of backward (state1 > state2) transitions for
    // each length. Index of vector == length of the transition. Thus zeroth element
    // contains nothing and should not be used.
    std::vector<float> totalNumbersOfBackwardTransitions;

    // Contains total number of self (state1 == state2) transitions
    float totalNumberOfSelfTransitions = 0;

    // Contatins total number of collected transitions
    size_t totalNumberOfTransitions = 0;

    // Contains total number of states
    size_t numStates = 0;

    void addState() override;

    void accumulateTransition(size_t state1, size_t state2) override;

    void getTransitionProbabilitiesEstimate(size_t state, Vector<float>* transitions) override;

    float getTransitionProbabilityEstimate(size_t state1, size_t state2) const override;
};

// Stats accumulator implementation, which employs transitions stats taking into account
// only the initial and final states of transitions.
class StatesBasedStatsAccumulator : public StatsAccumulator {
public:
    std::vector<float> transitionCounters;
    size_t totalNumberOfTransitions = 0;

    void addState() override;

    void accumulateTransition(size_t, size_t state2) override;

    // Basically this method yields the average probabilities of transitions to states, i.e.
    // it represents the "popularity" of states.
    void getTransitionProbabilitiesEstimate(size_t, Vector<float>* transitions) override;

    float getTransitionProbabilityEstimate(size_t, size_t state2) const override;
};