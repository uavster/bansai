#include "create_head_modulated_trajectory_view_action_handler.h"
#include "trajectory_store.h"

bool CreateHeadModulatedTrajectoryViewActionHandler::Run() {
  switch(state_) {
    case kProcessingRequest: {
      const P2PCreateHeadModulatedTrajectoryViewRequest &request = GetRequest();

      const int modulated_trajectory_view_id = static_cast<int>(NetworkToLocal<kP2PLocalEndianness>(request.id));
      const auto carrier_trajectory_view_type = static_cast<P2PTrajectoryViewType>(NetworkToLocal<kP2PLocalEndianness>(request.trajectory_view.carrier_trajectory_view_id.type));
      const auto carrier_trajectory_view_id = static_cast<int>(NetworkToLocal<kP2PLocalEndianness>(request.trajectory_view.carrier_trajectory_view_id.id));
      const auto modulator_trajectory_view_type = static_cast<P2PTrajectoryViewType>(NetworkToLocal<kP2PLocalEndianness>(request.trajectory_view.modulator_trajectory_view_id.type));
      const auto modulator_trajectory_view_id = static_cast<int>(NetworkToLocal<kP2PLocalEndianness>(request.trajectory_view.modulator_trajectory_view_id.id));
      const auto envelope_trajectory_view_id = static_cast<int>(NetworkToLocal<kP2PLocalEndianness>(request.trajectory_view.envelope_trajectory_view_id));

      char str[200];
      sprintf(str, "create_head_modulated_trajectory_view(id=%d, carrier_trajectory_view_id=%d, modulator_trajectory_view_id=%d, envelope_trajectory_view_id=%d)", modulated_trajectory_view_id, carrier_trajectory_view_id, modulator_trajectory_view_id, envelope_trajectory_view_id);
      LOG_INFO(str);

      auto &maybe_modulated_trajectory_view = trajectory_store_.head_modulated_trajectory_views()[modulated_trajectory_view_id];
      if (maybe_modulated_trajectory_view.status() == Status::kDoesNotExistError) {
        result_ = maybe_modulated_trajectory_view.status();
      } else {
        result_ = Status::kSuccess;
        const TrajectoryViewInterface<HeadTargetState> *carrier_view = nullptr;
        const TrajectoryViewInterface<HeadTargetState> *modulator_view = nullptr;
        const EnvelopeTrajectoryView *envelope_view = nullptr;


        switch(carrier_trajectory_view_type) {
          case P2PTrajectoryViewType::kPlain: {
            const auto &maybe_carrier_trajectory_view = trajectory_store_.head_trajectory_views()[carrier_trajectory_view_id];
            if (!maybe_carrier_trajectory_view.ok()) {
              result_ = maybe_carrier_trajectory_view.status();
            } else {
              carrier_view = &*maybe_carrier_trajectory_view;
            }
          }
          case P2PTrajectoryViewType::kModulated: {
            const auto &maybe_carrier_trajectory_view = trajectory_store_.head_modulated_trajectory_views()[carrier_trajectory_view_id];
            if (!maybe_carrier_trajectory_view.ok()) {
              result_ = maybe_carrier_trajectory_view.status();
            } else {
              carrier_view = &*maybe_carrier_trajectory_view;
            }
          }
          case P2PTrajectoryViewType::kMixed: {
            const auto &maybe_carrier_trajectory_view = trajectory_store_.head_mixed_trajectory_views()[carrier_trajectory_view_id];
            if (!maybe_carrier_trajectory_view.ok()) {
              result_ = maybe_carrier_trajectory_view.status();
            } else {
              carrier_view = &*maybe_carrier_trajectory_view;
            }
          }
        }

        if (result_ == Status::kSuccess) {
          switch(modulator_trajectory_view_type) {
            case P2PTrajectoryViewType::kPlain: {
              const auto &maybe_second_trajectory_view = trajectory_store_.head_trajectory_views()[modulated_trajectory_view_id];
              if (!maybe_second_trajectory_view.ok()) {
                result_ = maybe_second_trajectory_view.status();
              } else {
                modulator_view = &*maybe_second_trajectory_view;
              }
            }
            case P2PTrajectoryViewType::kModulated: {
              const auto &maybe_second_trajectory_view = trajectory_store_.head_modulated_trajectory_views()[modulated_trajectory_view_id];
              if (!maybe_second_trajectory_view.ok()) {
                result_ = maybe_second_trajectory_view.status();
              } else {
                modulator_view = &*maybe_second_trajectory_view;
              }
            }
            case P2PTrajectoryViewType::kMixed: {
              const auto &maybe_second_trajectory_view = trajectory_store_.head_mixed_trajectory_views()[modulated_trajectory_view_id];
              if (!maybe_second_trajectory_view.ok()) {
                result_ = maybe_second_trajectory_view.status();
              } else {
                modulator_view = &*maybe_second_trajectory_view;
              }
            }
          }
        }

        if (result_ == Status::kSuccess) {
          const auto maybe_alpha_envelope_trajectory_view = trajectory_store_.envelope_trajectory_views()[envelope_trajectory_view_id];
          if (!maybe_alpha_envelope_trajectory_view.ok()) {
            result_ = maybe_alpha_envelope_trajectory_view.status();
          } else {
            envelope_view = &*maybe_alpha_envelope_trajectory_view;
          }
        }

        if (result_ == Status::kSuccess) {
          result_ = Status::kSuccess;
          maybe_modulated_trajectory_view = HeadModulatedTrajectoryView();
          maybe_modulated_trajectory_view->carrier(carrier_view);
          maybe_modulated_trajectory_view->modulator(modulator_view);
          maybe_modulated_trajectory_view->envelope(envelope_view);
        }
      }
      if (TrySendingReply()) {
        return false; // Reply sent; do not call Run() again.
      }
      state_ = kSendingReply;
      break;
    }
    
    case kSendingReply: {
      if (TrySendingReply()) {
        state_ = kProcessingRequest;
        return false; // Reply sent; do not call Run() again.
      }
      break;
    }
  }
  return true;
}

bool CreateHeadModulatedTrajectoryViewActionHandler::TrySendingReply() {
  StatusOr<P2PActionPacketAdapter<P2PCreateHeadModulatedTrajectoryViewReply>> maybe_reply = NewReply();
  if (!maybe_reply.ok()) {
    return false;
  }
  P2PActionPacketAdapter<P2PCreateHeadModulatedTrajectoryViewReply> reply = *maybe_reply;
  reply->status_code = LocalToNetwork<kP2PLocalEndianness>(result_);
  reply.Commit(/*guarantee_delivery=*/true);
  return true;
}
