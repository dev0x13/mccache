#include <iostream>

#include "math/evolving_markov_chain.h"

EvolvingMarkovChain::EvolvingMarkovChain(const std::string& statsAccumulatorType, size_t accessesThreshold)
    : numStates(0), accessesThreshold(accessesThreshold)
{
    assert(statsAccumulatorType == "states" || statsAccumulatorType == "transitions");

    if (statsAccumulatorType == "transitions") {
        statsAccumulator = new TransitionsBasedStatsAccumulator();
    } else if (statsAccumulatorType == "states") {
        statsAccumulator = new StatesBasedStatsAccumulator();
    }
}

size_t EvolvingMarkovChain::addState() {
    ++numStates;

    // 1. Resize the stats matrices and fill the new values with zeroes

    transitionsStatsMatrix.resize(numStates);
    statesAccessesCounters.resize(numStates);

    transitionsStatsMatrix[numStates - 1].resize(numStates, 0);

    for (size_t i = 0; i < numStates - 1; ++i) {
        transitionsStatsMatrix[i].push_back(0);
    }

    // 1.1. Expire the stohastic matrix contents

    needToUpdateStohasticMatrix = true;

    // 2. Update the stats accumulator

    statsAccumulator->addState();

    return numStates - 1;
}

void EvolvingMarkovChain::registerTransition(size_t state1, size_t state2) {
    assert(state1 < numStates);
    assert(state2 < numStates);

    // 1. Update stats matrices

    transitionsStatsMatrix[state1][state2] += 1;
    statesAccessesCounters[state1] += 1;

    // 1.1. Expire the stohastic matrix contents

    needToUpdateStohasticMatrix = true;

    // 2. Update the stats accumulator

    statsAccumulator->accumulateTransition(state1, state2);
}

void EvolvingMarkovChain::predictNextState(size_t currentStateNum, Vector<float>* nextState) {
    assert(currentStateNum < numStates);
    assert(nextState);
    assert(nextState->getSize() == numStates);

    if (statesAccessesCounters[currentStateNum] < accessesThreshold) {
        // If we decide that collected transitions number from the given state is not
        // enough to give a prediction, we generate a prediction base on overall statistics
        // using stats accumulator.
        statsAccumulator->getTransitionProbabilitiesEstimate(currentStateNum, nextState);
    } else {
        // Otherwise just return the row from transitions matrix.
        nextState->copyFromVector(Vector<float>{ transitionsStatsMatrix[currentStateNum].data(), transitionsStatsMatrix[currentStateNum].size() });
    }
}

Vector<float> EvolvingMarkovChain::predictNextState(const Vector<float>& currentState) {
    assert(currentState.getSize() == numStates);

    updateStohasticMatrix();

    Vector<float> nextState(numStates);

    stohasticMatrix.matMulVec(currentState, &nextState);

    return nextState;
}

const Matrix<float>& EvolvingMarkovChain::getStohasticMatrix() {
    updateStohasticMatrix();
    return stohasticMatrix;
}

const size_t& EvolvingMarkovChain::getNumStates() const {
    return numStates;
}

void EvolvingMarkovChain::printTransitionsStatsMatrix() const {
    for (size_t i = 0; i < numStates; ++i) {
        std::cout << "[";

        for (size_t j = 0; j < numStates; ++j) {
            std::cout << " " << transitionsStatsMatrix[i][j];
        }

        std::cout << " ]\n";
    }

    std::cout << std::endl;
}

float EvolvingMarkovChain::getTransitionProbabilityFromAccumulator(size_t state1, size_t state2) const {
    assert(state1 < numStates);
    assert(state2 < numStates);

    return statsAccumulator->getTransitionProbabilityEstimate(state1, state2);
}

void EvolvingMarkovChain::updateStohasticMatrix() {
    // This check is useful when we are generating a prediction on the next states sequence
    // without changing the number of states and registering transitions.
    if (needToUpdateStohasticMatrix) {
        stohasticMatrix.resize(numStates, numStates, ResizeType::ZEROS);

        for (size_t i = 0; i < numStates; ++i) {
            Vector<float> rowView = stohasticMatrix.row(i);

            if (statesAccessesCounters[i] < accessesThreshold) {
                // If we decide that collected transitions number from the given state is not
                // enough to give a prediction, we generate a prediction base on overall statistics
                // using stats accumulator.
                statsAccumulator->getTransitionProbabilitiesEstimate(i, &rowView);
                rowView.scale(1.0 / rowView.sum());
            } else {
                // Otherwise just copy the corresponding row from the transitions matrix and normalize it.
                rowView.copyFromVector(Vector<float>{transitionsStatsMatrix[i].data(), numStates});
                rowView.scale(1.0 / statesAccessesCounters[i]);
            }
        }

        needToUpdateStohasticMatrix = false;
    }
}

EvolvingMarkovChain::~EvolvingMarkovChain() {
    delete statsAccumulator;
}