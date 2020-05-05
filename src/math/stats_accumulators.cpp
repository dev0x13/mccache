#include "math/stats_accumulators.h"

/************************************
 * TransitionsBasedStatsAccumulator *
 ************************************/

void TransitionsBasedStatsAccumulator::addState() {
    ++numStates;

    // We initialize new length with zeros
    totalNumbersOfForwardTransitions.push_back(0);
    totalNumbersOfBackwardTransitions.push_back(0);
}

void TransitionsBasedStatsAccumulator::accumulateTransition(size_t state1, size_t state2) {
    assert(state1 < numStates);
    assert(state2 < numStates);

    if (state1 == state2) {
        // Self-transition
        totalNumberOfSelfTransitions += 1;
    } else if (state1 < state2) {
        // Forward transition
        totalNumbersOfForwardTransitions[state2 - state1] += 1;
    } else if (state1 > state2) {
        // Backward transition
        totalNumbersOfBackwardTransitions[state1 - state2] += 1;
    }

    ++totalNumberOfTransitions;
}

void TransitionsBasedStatsAccumulator::getTransitionProbabilitiesEstimate(size_t state, Vector<float>* transitions) {
    assert(transitions);
    assert(transitions->getSize() == numStates);

    float *nextStateData = transitions->getData();
    const float* forwardTransitionsData  = totalNumbersOfForwardTransitions.data();

    /*
     * The transitions probabilities vector is filled as follows:
     *
     * < - - - - - - - - - - - - - <state> - - - - - - - - - - - - - - >
     *           ^                    ^                  ^
     * |backward transitions| |self transition| |forward transitions|
     *
     * So the transitions[N] element contains the probability of state -> N transition.
     */

    if (state != 0) {
        // Backward transitions are copied in reversed order as it contains transition lengths
        // in the ascending order.
        std::copy(
                totalNumbersOfBackwardTransitions.rbegin() + numStates - state - 1,
                totalNumbersOfBackwardTransitions.rend(),
                nextStateData
        );
    }

    if (state != numStates - 1) {
        std::copy(
                forwardTransitionsData + 1,
                forwardTransitionsData + (numStates - state),
                nextStateData + state + 1
        );
    }

    (*transitions)(state) = totalNumberOfSelfTransitions;

    // Normalize, but it doesn't mean that sum of elements is equal to 1 (!)
    transitions->scale(1.0 / totalNumberOfTransitions);
}

float TransitionsBasedStatsAccumulator::getTransitionProbabilityEstimate(size_t state1, size_t state2) const {
    assert(state1 < numStates);
    assert(state2 < numStates);

    if (state1 == state2) {
        return totalNumberOfSelfTransitions / totalNumberOfTransitions;
    } else if (state1 < state2) {
        return totalNumbersOfForwardTransitions[state2 - state1] / totalNumberOfTransitions;
    } else { /* if (state1 > state2) { */
        return totalNumbersOfBackwardTransitions[state1 - state2] / totalNumberOfTransitions;
    }
}

/*******************************
 * StatesBasedStatsAccumulator *
 *******************************/

void StatesBasedStatsAccumulator::addState() {
    transitionCounters.push_back(0);
    ++totalNumberOfTransitions;
}

void StatesBasedStatsAccumulator::accumulateTransition(size_t, size_t state2) {
    assert(state2 < transitionCounters.size());

    transitionCounters[state2] += 1;
}

// Basically this method yields the average probabilities of transitions to states, i.e.
// it represents the "popularity" of states.
void StatesBasedStatsAccumulator::getTransitionProbabilitiesEstimate(size_t, Vector<float>* transitions) {
    assert(transitions);
    assert(transitions->getSize() == transitionCounters.size());

    float *nextStateData = transitions->getData();
    const float* transitionsData = transitionCounters.data();

    std::copy(transitionsData, transitionsData + transitionCounters.size(), nextStateData);

    transitions->scale(1.0 / totalNumberOfTransitions);
}

float StatesBasedStatsAccumulator::getTransitionProbabilityEstimate(size_t, size_t state2) const {
    assert(state2 < transitionCounters.size());

    return transitionCounters[state2];
}
