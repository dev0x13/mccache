#pragma once

#include <cassert>
#include <cstddef>
#include <vector>

#include "vector.h"

// Stats accumulator interface. Used for collecting transitions statistics,
// collected with different transitions interpretations.
class StatsAccumulator {
 public:
  // Registers new state
  virtual void AddState() = 0;

  // Collects transition from state1 to state2
  virtual void AccumulateTransition(size_t state1, size_t state2) = 0;

  // Returns non-normalized (i.e. the sum of elements != 1) posterior
  // transitions probabilities from given state. Output transitions vector
  // should be pre-allocated to have <number of states> size.
  virtual void GetTransitionProbabilitiesEstimate(
      size_t state, Vector<float>* transitions) = 0;

  // Returns a single non-normalized posterior transition probability from
  // state1 to state2
  virtual float GetTransitionProbabilityEstimate(size_t state1,
                                                 size_t state2) const = 0;

  virtual ~StatsAccumulator() = default;
};

// Stats accumulator implementation, which employs transitions stats taking into
// account only the "length" of transitions (i.e. |state1 - state2|).
class TransitionsBasedStatsAccumulator : public StatsAccumulator {
 public:
  // Contains total numbers of forward (state1 < state2) transitions for
  // each length. Index of vector == length of the transition. Thus, zeroth
  // element contains nothing and should not be used.
  std::vector<float> total_numbers_of_forward_transitions_;

  // Contains total numbers of backward (state1 > state2) transitions for
  // each length. Index of vector == length of the transition. Thus, zeroth
  // element contains nothing and should not be used.
  std::vector<float> total_numbers_of_backward_transitions_;

  // Contains total number of self (state1 == state2) transitions
  float total_number_of_self_transitions_ = 0;

  // Contains total number of collected transitions
  size_t total_number_of_transitions_ = 0;

  // Contains total number of states
  size_t num_states_ = 0;

  void AddState() override;

  void AccumulateTransition(size_t state1, size_t state2) override;

  void GetTransitionProbabilitiesEstimate(size_t state,
                                          Vector<float>* transitions) override;

  float GetTransitionProbabilityEstimate(size_t state1,
                                         size_t state2) const override;
};

// Stats accumulator implementation, which employs transitions stats taking into
// account only the initial and final states of transitions.
class StatesBasedStatsAccumulator : public StatsAccumulator {
 public:
  std::vector<float> transition_counters_;
  size_t total_number_of_transitions_ = 0;

  void AddState() override;

  void AccumulateTransition(size_t, size_t state2) override;

  // Basically this method yields the average probabilities of transitions to
  // states, i.e. it represents the "popularity" of states.
  void GetTransitionProbabilitiesEstimate(size_t,
                                          Vector<float>* transitions) override;

  float GetTransitionProbabilityEstimate(size_t, size_t state2) const override;
};
