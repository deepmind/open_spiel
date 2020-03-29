# Copyright 2019 DeepMind Technologies Ltd. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Lint as: python3
"""Meta-strategy solvers for PSRO."""

import numpy as np

from open_spiel.python.algorithms import lp_solver
from open_spiel.python.algorithms import projected_replicator_dynamics
from open_spiel.python.algorithms.nash_solver import general_nash_solver as gs
import pyspiel


EPSILON_MIN_POSITIVE_PROBA = 1e-8


def uniform_strategy(solver, return_joint=False):
  """Returns a Random Uniform distribution on policies.

  Args:
    solver: GenPSROSolver instance.
    return_joint: If true, only returns marginals. Otherwise marginals as well
      as joint probabilities.

  Returns:
    uniform distribution on strategies.
  """
  policies = solver.get_policies()
  policy_lengths = [len(pol) for pol in policies]
  result = [np.ones(pol_len) / pol_len for pol_len in policy_lengths]
  if not return_joint:
    return result
  else:
    joint_strategies = get_joint_strategy_from_marginals(result)
    return result, joint_strategies


def softmax_on_range(number_policies):
  x = np.array(list(range(number_policies)))
  x = np.exp(x-x.max())
  x /= np.sum(x)
  return x


def uniform_biased_strategy(solver, return_joint=False):
  """Returns a Biased Random Uniform distribution on policies.

  The uniform distribution is biased to prioritize playing against more recent
  policies (Policies that were appended to the policy list later in training)
  instead of older ones.

  Args:
    solver: GenPSROSolver instance.
    return_joint: If true, only returns marginals. Otherwise marginals as well
      as joint probabilities.

  Returns:
    uniform distribution on strategies.
  """
  policies = solver.get_policies()
  if not isinstance(policies[0], list):
    policies = [policies]
  policy_lengths = [len(pol) for pol in policies]
  result = [softmax_on_range(pol_len) for pol_len in policy_lengths]
  if not return_joint:
    return result
  else:
    joint_strategies = get_joint_strategy_from_marginals(result)
    return result, joint_strategies


def renormalize(probabilities):
  """Replaces all negative entries with zeroes and normalizes the result.

  Args:
    probabilities: probability vector to renormalize. Has to be one-dimensional.

  Returns:
    Renormalized probabilities.
  """
  probabilities[probabilities < 0] = 0
  probabilities = probabilities / np.sum(probabilities)
  return probabilities


def get_joint_strategy_from_marginals(probabilities):
  """Returns a joint strategy matrix from a list of marginals.

  Args:
    probabilities: list of probabilities.

  Returns:
    A joint strategy from a list of marginals.
  """
  probas = []
  for i in range(len(probabilities)):
    probas_shapes = [1] * len(probabilities)
    probas_shapes[i] = -1
    probas.append(probabilities[i].reshape(*probas_shapes))
  result = np.product(probas)
  return result.reshape(-1)


def nash_strategy(solver, return_joint=False):
  """Returns nash distribution on meta game matrix.

  This method only works for two player zero-sum games.

  Args:
    solver: GenPSROSolver instance.
    return_joint: If true, only returns marginals. Otherwise marginals as well
      as joint probabilities.

  Returns:
    Nash distribution on strategies.
  """
  meta_games = solver.get_meta_game()
  if not isinstance(meta_games, list):
    meta_games = [meta_games, -meta_games]
  meta_games = [x.tolist() for x in meta_games]
  if len(meta_games) != 2:
    raise NotImplementedError(
        "nash_strategy solver works only for 2p zero-sum"
        "games, but was invoked for a {} player game".format(len(meta_games)))
  nash_prob_1, nash_prob_2, _, _ = (
      lp_solver.solve_zero_sum_matrix_game(
          pyspiel.create_matrix_game(*meta_games)))
  result = [
      renormalize(np.array(nash_prob_1).reshape(-1)),
      renormalize(np.array(nash_prob_2).reshape(-1))
  ]

  if not return_joint:
    return result
  else:
    joint_strategies = get_joint_strategy_from_marginals(result)
    return result, joint_strategies

#TODO: Test this sovler with PSRO.
def general_nash_strategy(solver, return_joint=False, NE_solver="gambit", mode='one'):
  """Returns nash distribution on meta game matrix.

    This method works for general-sum multi-player games.

    Args:
      solver: GenPSROSolver instance.
      return_joint: If true, only returns marginals. Otherwise marginals as well
        as joint probabilities.
      NE_solver: Tool for finding a NE.
      mode: Return one or all or pure NE.

    Returns:
      Nash distribution on strategies.
    """
  meta_games = solver.get_meta_game()
  if not isinstance(meta_games, list):
    meta_games = [meta_games, -meta_games]
  equilibria = gs.nash_solver(meta_games, solver=NE_solver, mode=mode)

  if not return_joint:
    return equilibria
  else:
    if len(equilibria) == 1:
      joint_strategies = get_joint_strategy_from_marginals(equilibria)
      return equilibria, joint_strategies
    else:
      # If multiple NE exist, return a list with joint strategies.
      joint_strategies_list = [get_joint_strategy_from_marginals([ne]) for ne in equilibria]
      return equilibria, joint_strategies_list


def prd_strategy(solver, return_joint=False):
  """Computes Projected Replicator Dynamics strategies.
  Args:
    solver: GenPSROSolver instance.
    return_joint: If true, only returns marginals. Otherwise marginals as well
      as joint probabilities.

  Returns:
    PRD-computed strategies.
  """
  meta_games = solver.get_meta_game()
  if not isinstance(meta_games, list):
    meta_games = [meta_games, -meta_games]
  kwargs = solver.get_kwargs()
  result = projected_replicator_dynamics.projected_replicator_dynamics(
      meta_games, **kwargs)
  if not return_joint:
    return result
  else:
    joint_strategies = get_joint_strategy_from_marginals(result)
    return result, joint_strategies


def self_play_strategy(solver, return_joint=False):
  """
  Return a strategy with only the newest strategy in the support (played with probability 1).
  :param solver: GenPSROSolver instance.
  :param return_joint: If true, only returns marginals. Otherwise marginals as well
      as joint probabilities.
  :return:
  """
  policies = solver.get_policies()
  policy_lengths = [len(pol) for pol in policies]
  result = []
  for pol_len in policy_lengths:
    strategy = np.zeros(pol_len)
    strategy[-1] = 1
    result.append(strategy)
  if not return_joint:
    return result
  else:
    joint_strategies = get_joint_strategy_from_marginals(result)
    return result, joint_strategies


META_STRATEGY_METHODS = {
    "uniform_biased": uniform_biased_strategy,
    "uniform": uniform_strategy,
    "nash": nash_strategy,
    "prd": prd_strategy,
    "general_nash": general_nash_strategy,
    "sp": self_play_strategy
}
