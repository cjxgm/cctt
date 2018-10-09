// No #pragma once intentionally

// Terminal output styling.

#define STYLE_START_        "\x1b["
#define STYLE_END_          "m"

#define STYLE_NORMAL        STYLE_START_ "0" STYLE_END_
#define STYLE_LOCATION      STYLE_START_ "1;36" STYLE_END_
#define STYLE_SOURCE        STYLE_START_ "1;35" STYLE_END_
#define STYLE_PATH          STYLE_START_ "0;33" STYLE_END_
#define STYLE_ERROR         STYLE_START_ "1;31" STYLE_END_
#define STYLE_BLOCK         STYLE_START_ "1;34" STYLE_END_
#define STYLE_LINK          STYLE_START_ "1;35" STYLE_END_

