# Set up the ESP-IDF environment (get_idf is an interactive shell alias and is not
# available in this non-interactive script, so source export.sh directly).
. "${IDF_PATH:-$HOME/esp/esp-idf}/export.sh"
# Remove any stale sdkconfig so sdkconfig.defaults (TEST mode) takes effect.
rm -f sdkconfig
idf.py build && idf.py flash && idf.py monitor 
