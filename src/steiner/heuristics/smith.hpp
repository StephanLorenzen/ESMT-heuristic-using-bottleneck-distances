#ifndef ITERATIVE_SMITH_H
#define ITERATIVE_SMITH_H

#include <vector>

#include "steiner/graph.hpp"
#include "steiner/heuristics/subgraph_heuristic.hpp"
#include "steiner/iterative.hpp"
#include "steiner/utils/point.hpp"

#define NMAX 11

/**
 * @class IterativeSmith
 *
 * Smith's iterative algorithm for finding the SMT of small graphs (N <= 10).
 * Implementation is taken from:
 * 
 *   W. D. Smith, How to find steiner minimal trees in euclidean d-space,
 *         Algorithmica 7 (1992), 137–177
 */
class IterativeSmith : public SubgraphHeuristic, public Iterative {
public:
  /**
   * Constructor
   */
  IterativeSmith(int dim, bool doCleanUp = false);
  
  /**
   * Destructor
   */
  ~IterativeSmith();

  /**
   * Finds the Steiner points and edges using Smith's iterative algorithm.
   */
  void findSteinerPoints(SteinerTree &subgraph);

  /**
   * Setter for the doCleanUp flag
   *
   * @param doCleanUp  the new value of doCleanUp
   */
  void setDoCleanUp(bool doCleanUp);

  /**
   * Setter for the doSortInput flag. This has no effect for numerical optimisation
   *
   * @param doSortInput  the new value of doSortInput
   */
  void setDoSortInput(bool doSortInput);
  
protected:
private:
  void buildTree(unsigned int l, int *topo_vec);

  bool do_clean_up;
};

#endif // ITERATIVE_SMITH_H
