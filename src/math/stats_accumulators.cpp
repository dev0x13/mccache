#include "math/stats_accumulators.h"

/************************************
 * TransitionsBasedStatsAccumulator *
 ************************************/

void TransitionsBasedStatsAccumulator::AddState() {
  ++num_states_;

  // We initialize new length with zeros
  total_numbers_of_forward_transitions_.push_back(0);
  total_numbers_of_backward_transitions_.push_back(0);
}

void TransitionsBasedStatsAccumulator::AccumulateTransition(size_t state1,
                                                            size_t state2) {
  assert(state1 < num_states_);
  assert(state2 < num_states_);

  if (state1 == state2) {
    // Self-transition
    total_number_of_self_transitions_ += 1;
  } else if (state1 < state2) {
    // Forward transition
    total_numbers_of_forward_transitions_[state2 - state1] += 1;
  } else if (state1 > state2) {
    // Backward transition
    total_numbers_of_backward_transitions_[state1 - state2] += 1;
  }

  ++total_number_of_transitions_;
}

void TransitionsBasedStatsAccumulator::GetTransitionProbabilitiesEstimate(
    size_t state, Vector<float>* transitions) {
  assert(transitions);
  assert(transitions->GetSize() == num_states_);

  float* next_state_data = transitions->GetData();
  const float* forward_transition_data =
      total_numbers_of_forward_transitions_.data();

  /*
   * The transitions probabilities vector is filled as follows:
   *
   * < - - - - - - - - - - - - - <state> - - - - - - - - - - - - - - >
   *           ^                    ^                  ^
   * |backward transitions| |self transition| |forward transitions|
   *
   * So the transitions[N] element contains the probability of state -> N
   * transition.
   */

  if (state != 0) {
    // Backward transitions are copied in reversed order as it contains
    // transition lengths in the ascending order.
    std::copy(total_numbers_of_backward_transitions_.rbegin() + num_states_ -
                  state - 1,
              total_numbers_of_backward_transitions_.rend(), next_state_data);
  }

  if (state != num_states_ - 1) {
    std::copy(forward_transition_data + 1,
              forward_transition_data + (num_states_ - state),
              next_state_data + state + 1);
  }

  (*transitions)(state) = total_number_of_self_transitions_;

  // Normalize, but it doesn't mean that sum of elements is equal to 1 (!)
  transitions->Scale(1.0 / total_number_of_transitions_);
}

float TransitionsBasedStatsAccumulator::GetTransitionProbabilityEstimate(
    size_t state1, size_t state2) const {
  assert(state1 < num_states_);
  assert(state2 < num_states_);

  if (state1 == state2) {
    return total_number_of_self_transitions_ / total_number_of_transitions_;
  } else if (state1 < state2) {
    return total_numbers_of_forward_transitions_[state2 - state1] /
           total_number_of_transitions_;
  } else { /* if (state1 > state2) { */
    return total_numbers_of_backward_transitions_[state1 - state2] /
           total_number_of_transitions_;
  }
}

/*******************************
 * StatesBasedStatsAccumulator *
 *******************************/

void StatesBasedStatsAccumulator::AddState() {
  transition_counters_.push_back(0);
  ++total_number_of_transitions_;
}

void StatesBasedStatsAccumulator::AccumulateTransition(size_t, size_t state2) {
  assert(state2 < transition_counters_.size());

  transition_counters_[state2] += 1;
}

// Basically this method yields the average probabilities of transitions to
// states, i.e. it represents the "popularity" of states.
void StatesBasedStatsAccumulator::GetTransitionProbabilitiesEstimate(
    size_t, Vector<float>* transitions) {
  assert(transitions);
  assert(transitions->GetSize() == transition_counters_.size());

  float* next_state_data = transitions->GetData();
  const float* transitions_data = transition_counters_.data();

  std::copy(transitions_data, transitions_data + transition_counters_.size(),
            next_state_data);

  transitions->Scale(1.0 / total_number_of_transitions_);
}

float StatesBasedStatsAccumulator::GetTransitionProbabilityEstimate(
    size_t, size_t state2) const {
  assert(state2 < transition_counters_.size());

  return transition_counters_[state2];
}
