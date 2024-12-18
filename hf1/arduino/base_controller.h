#ifndef ROBOT_SPEED_CONTROLLER_
#define ROBOT_SPEED_CONTROLLER_

#include "wheel_controller.h"
#include "base_state.h"
#include "timer.h"
#include "trajectory_view.h"
#include "controller.h"
#include "modulated_trajectory_view.h"

// Controller commanding the wheel speed controllers to achieve the desired forward and 
// angular speeds of the robot's base.
class BaseSpeedController {
public:
  // Does not take ownership of any pointee.
  BaseSpeedController(WheelSpeedController *left_wheel, WheelSpeedController *right_wheel);

  // Sets the target linear and angular speeds. The speeds might not be attainable by the 
  // wheels. If the linear speed is not attainable, the target is clamped to the wheel limit. 
  // Then, if the angular speed is not attainable, it is clamped to the angular speed limit 
  // given the updated target linear speed, and the linear speed is adjusted to keep the 
  // radius given by the updated linear speed and the original angular speed.
  void SetTargetSpeeds(float linear, float angular);

  // Returns the target linear speed adjusted to what the wheels can do.
  float target_linear_speed() const { return target_speed_linear_; }
  // Returns the target angular speed adjusted to what the wheels can do.
  float target_angular_speed() const { return target_speed_angular_; }
  // Returns the radius of the curve given the adjusted target speeds.
  float curve_radius() const { return target_linear_speed() / target_angular_speed(); }

  WheelSpeedController &left_wheel_speed_controller() const { return left_wheel_; }
  WheelSpeedController &right_wheel_speed_controller() const { return right_wheel_; }

private:
  WheelSpeedController &left_wheel_;
  WheelSpeedController &right_wheel_;
  float target_speed_linear_;
  float target_speed_angular_;
};

// Controller commanding the base speed controller to achieve the desired position and yaw
// of the robot's base within a tolerance. Convergence is not guaranteed in any given time
// horizon.
//
// This controller is based on:
// Y. Kanayama, Y. Kimura, F. Miyazaki, and T. Noguchi, "A stable tracking control method 
// for an autonomous mobile robot," Proc. IEEE Int. Conf. Rob. Autom., 1990, pp. 384–389.
class BaseStateController : public Controller {
public:
  BaseStateController(const char *name, BaseSpeedController *base_speed_controller);

  void SetTargetState(const Point &center_position_target, float yaw_target, float reference_forward_speed, float reference_angular_speed = 0.0f);

  const BaseSpeedController &base_speed_controller() const { return base_speed_controller_; }

  bool IsAtTargetState() const;

protected:
  void StopControl() override;
  void Update(TimerSecondsType now_seconds) override;

private:
  BaseSpeedController &base_speed_controller_;
  Point center_position_target_;
  float yaw_target_;
  float reference_forward_speed_;
  float reference_angular_speed_;
};

using BaseTargetState = State<BaseStateVars, 0>;

// Defines the state of the robot's base at a given time. The controller class decides what
// part of the state to use. For instance, some controllers may ignore the time and/or the 
// yaw angle.
using BaseWaypoint = Waypoint<BaseTargetState>;

// A view of a collection of base waypoints.
// The view does not own the memory containing the waypoints, so they must outlive any 
// view objects referencing them.
using BaseTrajectoryView = TrajectoryView<BaseTargetState>;

class BaseModulatedTrajectoryView : public ModulatedTrajectoryView<BaseTargetState> {
public:
  // Returns the waypoint at the given index, after applying interpolation.
  BaseWaypoint GetWaypoint(float seconds) const override;
};


// Controller commanding the base speed controller to move the robot's base over a sequence
// of waypoints.
// 
// The feedforward and feedback terms are taken from:
// R. L. S. Sousa, M. D. do Nascimento Forte, F. G. Nogueira, B. C. Torrico, 
// “Trajectory tracking control of a nonholonomic mobile robot with differential drive”, 
// in Proc. IEEE Biennial Congress of Argentina (ARGENCON), pp. 1–6, 2016.
//
// The position, velocity and acceleration references are linearly interpolated from the
// current and next target waypoints by looking ahead in the trajectory. This reduces the
// tracking error due to the base's maximum dynamics (see base class' documentation for more
// information).
//
// The base will not drive over a waypoint if it was not able to reach it on time. A far 
// waypoint's state with a time very near to the previous waypoint's time will not be 
// reachable, either because the robot's maximum acceleration and speed are insufficient, 
// or because the time between waypoints is under the controller's sampling period (0.1 
// seconds).
//
// Also, any obstacle and driving hurdle or error can result in not reaching a waypoint in 
// time. When the waypoint's time constraint cannot be met and the waypoint is the last one 
// in the trajectory, the robot will stop. But if the waypoint is not the last one, the 
// robot will skip to the next one.
class BaseTrajectoryController : public TrajectoryController<BaseTargetState> {
public:
  BaseTrajectoryController(const char *name, BaseSpeedController *base_speed_controller);

  // Returns the underlying base speed controller.
  const BaseSpeedController &base_speed_controller() const { return base_speed_controller_; }

protected:
  virtual void Update(TimerSecondsType seconds_since_start) override;
  virtual void StopControl() override;

private:
  BaseSpeedController &base_speed_controller_;
};

#endif  // ROBOT_SPEED_CONTROLLER_