#include "math/evolving_markov_chain.h"

#include <iostream>

EvolvingMarkovChain::EvolvingMarkovChain(
    const std::string& stats_accumulator_type, size_t accesses_threshold)
    : num_states_(0), accesses_threshold_(accesses_threshold) {
  assert(stats_accumulator_type == "states" ||
         stats_accumulator_type == "transitions");

  if (stats_accumulator_type == "transitions") {
    stats_accumulator_ = new TransitionsBasedStatsAccumulator();
  } else if (stats_accumulator_type == "states") {
    stats_accumulator_ = new StatesBasedStatsAccumulator();
  }
}

size_t EvolvingMarkovChain::AddState() {
  ++num_states_;

  // 1. Resize the stats matrices and fill the new values with zeroes

  transition_stats_matrix_.resize(num_states_);
  states_access_counters_.resize(num_states_);

  transition_stats_matrix_[num_states_ - 1].resize(num_states_, 0);

  for (size_t i = 0; i < num_states_ - 1; ++i) {
    transition_stats_matrix_[i].push_back(0);
  }

  // 1.1. Expire the stohastic matrix contents

  need_to_update_stochastic_matrix_ = true;

  // 2. Update the stats accumulator

  stats_accumulator_->AddState();

  return num_states_ - 1;
}

void EvolvingMarkovChain::RegisterTransition(size_t state1, size_t state2) {
  assert(state1 < num_states_);
  assert(state2 < num_states_);

  // 1. Update stats matrices

  transition_stats_matrix_[state1][state2] += 1;
  states_access_counters_[state1] += 1;

  // 1.1. Expire the stohastic matrix contents

  need_to_update_stochastic_matrix_ = true;

  // 2. Update the stats accumulator

  stats_accumulator_->AccumulateTransition(state1, state2);
}

void EvolvingMarkovChain::PredictNextState(size_t current_state_num,
                                           Vector<float>* next_state) {
  assert(current_state_num < num_states_);
  assert(next_state);
  assert(next_state->GetSize() == num_states_);

  if (states_access_counters_[current_state_num] < accesses_threshold_) {
    // If we decide that collected transitions number from the given state is
    // not enough to give a prediction, we generate a prediction base on overall
    // statistics using stats accumulator.
    stats_accumulator_->GetTransitionProbabilitiesEstimate(current_state_num,
                                                           next_state);
  } else {
    // Otherwise just return the row from transitions matrix.
    next_state->CopyFromVector(
        Vector<float>{transition_stats_matrix_[current_state_num].data(),
                      transition_stats_matrix_[current_state_num].size()});
  }
}

Vector<float> EvolvingMarkovChain::PredictNextState(
    const Vector<float>& current_state) {
  assert(current_state.GetSize() == num_states_);

  UpdateStochasticMatrix();

  Vector<float> next_state(num_states_);

  stochastic_matrix_.TransMatMulVec(current_state, &next_state);

  return next_state;
}

const Matrix<float>& EvolvingMarkovChain::GetStochasticMatrix() {
  UpdateStochasticMatrix();
  return stochastic_matrix_;
}

const size_t& EvolvingMarkovChain::GetNumStates() const { return num_states_; }

void EvolvingMarkovChain::PrintTransitionsStatsMatrix() const {
  for (size_t i = 0; i < num_states_; ++i) {
    std::cout << "[";

    for (size_t j = 0; j < num_states_; ++j) {
      std::cout << " " << transition_stats_matrix_[i][j];
    }

    std::cout << " ]\n";
  }

  std::cout << std::endl;
}

float EvolvingMarkovChain::GetTransitionProbabilityFromAccumulator(
    size_t state1, size_t state2) const {
  assert(state1 < num_states_);
  assert(state2 < num_states_);

  return stats_accumulator_->GetTransitionProbabilityEstimate(state1, state2);
}

void EvolvingMarkovChain::UpdateStochasticMatrix() {
  // This check is useful when we are generating a prediction on the next states
  // sequence without changing the number of states and registering transitions.
  if (need_to_update_stochastic_matrix_) {
    stochastic_matrix_.Resize(num_states_, num_states_, ResizeType::kZeros);

    for (size_t i = 0; i < num_states_; ++i) {
      Vector<float> row_view = stochastic_matrix_.Row(i);

      if (states_access_counters_[i] < accesses_threshold_) {
        // If we decide that collected transitions number from the given state
        // is not enough to give a prediction, we generate a prediction base on
        // overall statistics using stats accumulator.
        stats_accumulator_->GetTransitionProbabilitiesEstimate(i, &row_view);
        row_view.Scale(1.0f / row_view.Sum());
      } else {
        // Otherwise just copy the corresponding row from the transitions matrix
        // and normalize it.
        row_view.CopyFromVector(
            Vector<float>{transition_stats_matrix_[i].data(), num_states_});
        row_view.Scale(1.0 / states_access_counters_[i]);
      }
    }

    need_to_update_stochastic_matrix_ = false;
  }
}

EvolvingMarkovChain::~EvolvingMarkovChain() { delete stats_accumulator_; }