#ifndef LEPP2_SMOOTH_OBSTACLE_AGGREGATOR_H__
#define LEPP2_SMOOTH_OBSTACLE_AGGREGATOR_H__

#include "lepp2/ObstacleAggregator.hpp"
#include <list>
#include <map>

namespace lepp {

/**
 * A visitor implementation that will perform a translation of models that it
 * visits by the given translation vector.
 */
class BlendVisitor : public ModelVisitor {
public:
  /**
   * Create a new `BlendVisitor` that will translate objects by the given
   * vector.
   */
  BlendVisitor(Coordinate translation_vec) : translation_vec_(translation_vec) {}
  void visitSphere(SphereModel& sphere) {
    sphere.set_center(sphere.center() + translation_vec_);
  }
  void visitCapsule(CapsuleModel& capsule) {
    capsule.set_first(capsule.first() + translation_vec_);
    capsule.set_second(capsule.second() + translation_vec_);
  }
private:
  Coordinate const translation_vec_;
};

/**
 * An `ObstacleAggregator` decorator.
 *
 * It takes the obstacles found by an obstacle detector and applies some
 * postprocessing in order to "smooth out" the obstacles being passed on to the
 * final output.
 *
 * It does so by tracking which of the obstacles detected in the new frame have
 * also been previously found and outputting only those that have been found in
 * a sufficient number of consecutive frames, so as to give us some certainty
 * that its appearance is not due to sensor noise.
 *
 * Conversely, it also tracks which obstacles have disappeared, propagating the
 * disappearance only if the obstacle has been gone in a sufficient number of
 * consecutive frames.
 *
 * It emits the obstacles that it considers real in each frame to all
 * aggregators that are attached to it.
 */
class SmoothObstacleAggregator : public ObstacleAggregator {
public:
  /**
   * Creates a new `SmoothObstacleAggregator`.
   */
  SmoothObstacleAggregator();
  /**
   * Attach a new `ObstacleAggregator` that will be notified of obstacles that
   * this instance generates.
   */
  void attachObstacleAggregator(boost::shared_ptr<ObstacleAggregator> aggreg);
  /**
   * The member function that all concrete aggregators need to implement in
   * order to be able to process newly detected obstacles.
   */
  virtual void updateObstacles(std::vector<ObjectModelPtr> const& obstacles);
private:
  // Private types
  /**
   * The type that represents model IDs. For convenience it aliases an int.
   */
  typedef int model_id_t;

  // Private member functions
  /**
   * Sends the given obstacles to all attached aggregators.
   */
  void notifyAggregators(std::vector<ObjectModelPtr> const& obstacles);
  /**
   * Computes the matching of the new obstacles to the obstacles that are being
   * tracked already.
   *
   * If a new obstacle does not have a match in the ones being tracked, a new
   * ID is assigned to it and it is added to the `tracked_models_`.
   *
   * The returned map represents a mapping of model IDs (found in the
   * `tracked_models_`) to the index of this obstacle in the `new_obstacles`
   * list.
   */
  std::map<model_id_t, size_t> matchToPrevious(
      std::vector<ObjectModelPtr> const& new_obstacles);
  /**
   * Adapts the currently tracked objects by taking into account their new
   * representations.
   */
  void adaptTracked(
      std::map<model_id_t, size_t> const& correspondence,
      std::vector<ObjectModelPtr> const& new_obstacles);
  /**
   * Updates the internal `frames_found_` and `frames_lost_` counters for each
   * mode, based on the given new matches description, i.e. increments the
   * seen counter for all models that were already tracked and found in the new
   * frame (i.e. included in the `new_matches`); increments the lost counter for
   * all models that were tracked, but not found in the new frame.
   *
   * The format of the given parameter is the one returned by the
   * `matchToPrevious` member function.
   */
  void updateLostAndFound(std::map<model_id_t, size_t> const& new_matches);
  /**
   * Drops any object that has been lost too many frames in a row.
   * This means that the object is removed from tracked objects, as well as no
   * longer returned as a "real" (materialized) object.
   */
  void dropLostObjects();
  /**
   * Materializes any object that has been seen enough frames in a row.
   * This means that tracked objects that seem to be stable are "graduated up"
   * to a "real" object and from there on out presented to the underlying
   * aggregator.
   */
  void materializeFoundObjects();
  /**
   * Convenience function that copies the list of materialized objects to a list
   * that can then be given to the underlying aggregator.
   */
   std::vector<ObjectModelPtr> copyMaterialized();
  /**
   * The function returns the next available model ID. It makes sure that no
   * models are ever assigned the same ID.
   */
  model_id_t nextModelId();
  /**
   * Returns the ID of a model tracked that matches the given model.
   *
   * The matching is done in such a way to return the model that whose
   * characteristic point has the smallest distance from the point of the given
   * mode, but given that the distance is below a certain threshold.
   *
   * Objects for which no such match can be found are given a new ID and this ID
   * is returned.
   */
  model_id_t getMatchByDistance(ObjectModelPtr model);

  // Private members
  /**
   * A list of aggregators to which this one will pass its own list of obstacles
   */
  std::vector<boost::shared_ptr<ObstacleAggregator> > aggregators_;
  /**
   * Keeps track of which model ID is the next one that can be assigned.
   */
  model_id_t next_model_id_;
  /**
   * A mapping of model IDs to their `ObjectModel` representation.
   */
  std::map<model_id_t, boost::shared_ptr<ObjectModel> > tracked_models_;
  /**
   * A mapping of the model ID to the number of subsequent frames that the model
   * was found in.
   */
  std::map<model_id_t, int> frames_found_;
  /**
   * A mapping of the model ID to the number of subsequent frames that the model
   * was no longer found in.
   */
  std::map<model_id_t, int> frames_lost_;
  /**
   * Contains those models that are currently considered "real", i.e. not simply
   * perceived in one frame, but with sufficient certainty in many frames that
   * we can claim it's a real object (therefore, it got "materialized").
   * We use a linked-list for this purpose since we need efficient removals
   * without copying elements or (importantly) invalidating iterators.
   */
  std::list<ObjectModelPtr> materialized_models_;
  /**
   * Maps the model to their position in the linked list so as to allow removing
   * objects efficiently.
   * Theoretically, the removal would asymptotically be dominated by the map
   * lookup (since it's a red-black tree, not a hash table). In practice, since
   * the number of objects will always be extremely small, it may as well be
   * O(1).
   */
  std::map<model_id_t, std::list<ObjectModelPtr>::iterator> model_idx_in_list_;
  /**
   * Current count of the number of frames processed by the aggregator.
   */
  int frame_cnt_;
};

SmoothObstacleAggregator::SmoothObstacleAggregator()
    : next_model_id_(0), frame_cnt_(0) {}

void SmoothObstacleAggregator::attachObstacleAggregator(
    boost::shared_ptr<ObstacleAggregator> aggregator) {
  aggregators_.push_back(aggregator);
}


SmoothObstacleAggregator::model_id_t SmoothObstacleAggregator::nextModelId() {
  return next_model_id_++;
}

SmoothObstacleAggregator::model_id_t
SmoothObstacleAggregator::getMatchByDistance(ObjectModelPtr model) {
  Coordinate const query_point = model->center_point();
  bool found = false;
  double min_dist = 1e10;
  model_id_t match = 0;
  for (auto const& elem : tracked_models_) {
    Coordinate const p(elem.second->center_point());
    double const dist =
        (p.x - query_point.x) * (p.x - query_point.x) +
        (p.y - query_point.y) * (p.y - query_point.y) +
        (p.z - query_point.z) * (p.z - query_point.z);
    std::cout << "Distance was " << dist << " ";
    if (dist <= 0.05) {
      std::cout << " accept";
      if (!found) min_dist = dist;
      found = true;
      if (dist <= min_dist) {
        min_dist = dist;
        match = elem.first;
      }
    }
    std::cout << std::endl;
  }

  if (found) {
    return match;
  } else {
    return nextModelId();
  }
}

std::map<SmoothObstacleAggregator::model_id_t, size_t>
SmoothObstacleAggregator::matchToPrevious(
    std::vector<ObjectModelPtr> const& new_obstacles) {
  // Maps the ID of the model to its index in the new list of obstacles.
  // This lets us know the new approximation of each currently tracked object.
  // If the model ID is not in the `tracked_models_`, it means we've got a new
  // object in the stream.
  std::map<model_id_t, size_t> correspondence;
  // Keeps a list of objects that were new in the frame (the ID and its index
  // in the `new_obstacles`). This is done in order to insert the new objects
  // after the matching step, since we don't want some of the new objects to
  // accidentaly get matched to one of the other new ones. Therefore, we defer
  // the update of the tracked objects 'till after the matching step.
  std::vector<std::pair<model_id_t, size_t> > new_in_frame;

  // First we match each new obstacle to one of the models that is currently
  // being tracked or give it a brand new model ID, if we are unable to find a
  // match.
  size_t i = 0;
  for (auto const& new_obstacle : new_obstacles) {
    std::cout << "Matching " << i << " (" << *new_obstacle << ")" << std::endl;
    model_id_t const model_id = getMatchByDistance(new_obstacle);
    std::cout << "Matched " << i << " --> " << model_id << std::endl;
    correspondence[model_id] = i;
    // If this one wasn't in the tracked models before, add it!
    if (tracked_models_.find(model_id) == tracked_models_.end()) {
      new_in_frame.push_back(std::make_pair(model_id, i));
    }
    ++i;
  }

  // We start tracking each obstacle for which we were unable to find a match
  // in the currently tracked list of objects.
  for (auto const& elem : new_in_frame) {
    model_id_t const& model_id = elem.first;
    size_t const corresp = elem.second;
    std::cout << "Inserting previously untracked model " << model_id << std::endl;
    tracked_models_[model_id] = new_obstacles[corresp];
    frames_lost_[model_id] = 0;
    frames_found_[model_id] = 0;
    // We assign it the ID here too!
    new_obstacles[corresp]->set_id(model_id);
  }

  return correspondence;
}

void SmoothObstacleAggregator::adaptTracked(
    std::map<model_id_t, size_t> const& correspondence,
    std::vector<ObjectModelPtr> const& new_obstacles) {
  for (auto const& elem : correspondence) {
    model_id_t const& model_id = elem.first;
    int const& i = elem.second;
    // Blend the new representation into the one we're tracking
    Coordinate const translation_vec =
        (new_obstacles[i]->center_point() - tracked_models_[model_id]->center_point()) / 2;
    BlendVisitor blender(translation_vec);
    tracked_models_[model_id]->accept(blender);
    if (frame_cnt_ % 30 == 0) {
      CompositeModel* tracked = dynamic_cast<CompositeModel*>(&*tracked_models_[model_id]);
      CompositeModel* new_model = dynamic_cast<CompositeModel*>(&*new_obstacles[i]);
      if (tracked && new_model) {
        tracked->set_models(new_model->models());
      }
    }
  }
}

void SmoothObstacleAggregator::updateLostAndFound(
    std::map<model_id_t, size_t> const& new_matches) {
  for (auto const& elem : tracked_models_) {
    model_id_t const& model_id = elem.first;
    if (new_matches.find(model_id) != new_matches.end()) {
      // Update the seen count only if the object isn't already materialized.
      if (model_idx_in_list_.find(model_id) == model_idx_in_list_.end()) {
        std::cout << "Incrementing the seen count for " << model_id << std::endl;
        ++frames_found_[model_id];
      }
      // ...but always reset its lost counter, since we've now seen it.
      frames_lost_[model_id] = 0;
    } else {
      std::cout << "Incrementing the lost count for " << model_id << std::endl;
      ++frames_lost_[model_id];
      frames_found_[model_id] = 0;
    }
  }
}

void SmoothObstacleAggregator::dropLostObjects() {
  // Drop obstacles that haven't been seen in a while
  // !!!NOTE!!! Deleting while iterating is no longer the same in C++11!
  auto it = frames_lost_.begin();
  int const LOST_LIMIT = 10;
  while (it != frames_lost_.end()) {
    if (it->second >= LOST_LIMIT) {
      std::cout << "Object " << it->first << " not found 5 times in a row: DROPPING" << std::endl;
      // Stop tracking the model, since it's been gone for a while.
      if (tracked_models_.find(it->first) != tracked_models_.end()) {
        tracked_models_.erase(tracked_models_.find(it->first));
      }
      // If the model was also materialized, make sure we drop it from there too
      if (model_idx_in_list_.find(it->first) != model_idx_in_list_.end()) {
        materialized_models_.erase(model_idx_in_list_[it->first]);
        model_idx_in_list_.erase(model_idx_in_list_.find(it->first));
      }
      // Remove the helper tracking data too.
      if (frames_found_.find(it->first) != frames_found_.end()) {
        frames_found_.erase(frames_found_.find(it->first));
      }
      frames_lost_.erase(it++);
    } else {
      std::cout << "Object " << it->first << " not lost enough times to be dropped"
        << " (" << it->second << ")"
        << std::endl;
      ++it;
    }
  }
}

void SmoothObstacleAggregator::materializeFoundObjects() {
  int const FOUND_LIMIT = 5;
  auto it = frames_found_.begin();
  while (it != frames_found_.end()) {
    // Deconstruct the iterator pair for convenience
    model_id_t const model_id = it->first;
    int const seen_count = it->second;
    if (seen_count >= FOUND_LIMIT) {
      std::cout << "Object found " << model_id << " 5 times in a row: INCLUDING!" << std::endl;
      // Get the corresponding model
      ObjectModelPtr const& model = tracked_models_.find(model_id)->second;
      // Materialize it...
      materialized_models_.push_back(model);
      // ...and make sure we know where in the list it got inserted.
      auto pos = materialized_models_.end();
      --pos;
      model_idx_in_list_[model_id] = pos;
      // Finally, make sure we remove it from the mapping so that we don't end
      // up adding it more than once to the list of materialized objects.
      frames_found_.erase(it++);
    } else {
      std::cout << "Object " << it->first << " not seen enough times to be included"
        << " (" << it->second << ")"
        << std::endl;
      ++it;
    }
  }

}

std::vector<ObjectModelPtr> SmoothObstacleAggregator::copyMaterialized() {
  std::vector<ObjectModelPtr> smooth_obstacles(
      materialized_models_.begin(), materialized_models_.end());
  return smooth_obstacles;
}

void SmoothObstacleAggregator::notifyAggregators(
    std::vector<ObjectModelPtr> const& obstacles) {
  for (auto& aggregator : aggregators_) {
    aggregator->updateObstacles(obstacles);
  }
}

void SmoothObstacleAggregator::updateObstacles(
    std::vector<ObjectModelPtr> const& obstacles) {
  ++frame_cnt_;
  std::cout << "#new = " << obstacles.size() << std::endl;

  auto correspondence = matchToPrevious(obstacles);
  updateLostAndFound(correspondence);
  adaptTracked(correspondence, obstacles);
  dropLostObjects();
  materializeFoundObjects();
  std::vector<ObjectModelPtr> smooth_obstacles(copyMaterialized());

  std::cout << "Real in this frame = " << smooth_obstacles.size() << std::endl;
  notifyAggregators(smooth_obstacles);
}

}  // namespace lepp

#endif
