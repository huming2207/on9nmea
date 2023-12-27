if(ESP_PLATFORM)
    idf_component_register(SRCS "on9nmea.c" "on9nmea.h"
            INCLUDE_DIRS ".")
else ()
    project("on9nmea")
    add_library("on9nmea" STATIC "on9nmea.c" "on9nmea.c")
    target_include_directories("on9nmea" PUBLIC ".")
endif ()