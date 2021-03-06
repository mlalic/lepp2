#ifndef GRABBER_VIDEO_SOURCE_H_
#define GRABBER_VIDEO_SOURCE_H_

#include "BaseVideoSource.hpp"
#include <pcl/io/openni_grabber.h>

namespace lepp {

/**
 * An implementation of the VideoSource abstract base class, based on PCL's
 * Grabber class.
 *
 * It allows clients to wrap a PCL Grabber to the VideoSource interface.
 * The grabber which is wrapped needs to be injected at construction of the
 * source instance.
 */
template<class PointT>
class GeneralGrabberVideoSource : public VideoSource<PointT> {
public:
  /**
   * Instantiate a video source which wraps the given Grabber instance.
   * The VideoSource instance takes ownership of the given Grabber instance.
   */
  GeneralGrabberVideoSource(boost::shared_ptr<pcl::Grabber> interface)
      : interface_(interface) {}
  virtual ~GeneralGrabberVideoSource();
  virtual void open();
private:
  /**
   * A reference to the Grabber instance that the VideoSource wraps.
   */
  const boost::shared_ptr<pcl::Grabber> interface_;

  /**
   * Member function which is registered as a callback of the Grabber.
   * Acts as the bond between the VideoSource and the Grabber, allowing the
   * adaptation of the interface.
   */
  void cloud_cb_(const typename pcl::PointCloud<PointT>::ConstPtr& cloud);
};

template<class PointT>
GeneralGrabberVideoSource<PointT>::~GeneralGrabberVideoSource() {
  // RAII: make sure to stop any running Grabber
  interface_->stop();
}

template<class PointT>
void GeneralGrabberVideoSource<PointT>::cloud_cb_(
    const typename pcl::PointCloud<PointT>::ConstPtr& cloud) {
  this->setNextFrame(cloud);
}

template<class PointT>
void GeneralGrabberVideoSource<PointT>::open() {
  // Register the callback and start grabbing frames...
  typedef void (callback_t)(const typename pcl::PointCloud<PointT>::ConstPtr&);
  boost::function<callback_t> f = boost::bind(
      &GeneralGrabberVideoSource::cloud_cb_,
      this, _1);
  interface_->registerCallback(f);

  interface_->start();
}

/**
 * A convenience class for a live stream captured from a local RGB-D sensor.
 *
 * The implementation leverages the GeneralGrabberVideoSource wrapping a
 * PCL-based OpenNIGrabber instance.
 */
template<class PointT>
class LiveStreamSource : public GeneralGrabberVideoSource<PointT> {
public:
  LiveStreamSource()
      : GeneralGrabberVideoSource<PointT>(boost::shared_ptr<pcl::Grabber>(
            new pcl::OpenNIGrabber("", pcl::OpenNIGrabber::OpenNI_QVGA_30Hz))) {
    // Empty... All work performed in the initializer list.
  }
};

}  // namespace lepp

#endif
