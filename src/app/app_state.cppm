module;
export module app_state;

import avatar_state;
import chat_state;
import permission_state;

export namespace jay {

class ApplicationState {
public:
  ApplicationState() = default;

  AvatarState avatar;
  ChatState chat;
  PermissionState permission;
};

} // namespace jay
