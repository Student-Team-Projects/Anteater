#include "html/utils.h"
#include <fmt/core.h>
#include <pwd.h>

std::string getUserInfoString(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    if (pw != nullptr) {
        return fmt::format("{} ({})", pw->pw_name, uid);
    } else {
        return fmt::format("({})", uid);
    }
}
