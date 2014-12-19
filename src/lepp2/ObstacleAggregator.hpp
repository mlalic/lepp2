#ifndef LEPP2_OBSTACLE_AGGREGATOR_H__
#define LEPP2_OBSTACLE_AGGREGATOR_H__

#include "lepp2/legacy/models/ObjectModel.hpp"

namespace lepp
{

/**
 * An interface that all classes that wish to be notified of obstacles detected
 * by an ObstacleDetector need to implement.
 *
 * The member function ``updateObstacles`` accepts a new list of obstacles
 * found by the detector.
 */
class ObstacleAggregator {
public:
  /**
   * The member function that all concrete aggregators need to implement in
   * order to be able to process newly detected obstacles.
   */
  virtual void updateObstacles(ObjectModelPtrListPtr obstacles) = 0;
};

} // namespace lepp
#endif
